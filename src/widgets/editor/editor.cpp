/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// #define DRAWSPRITE_DOUBLEBUFFERED

#include "config.h"

#include "widgets/editor/editor.h"

#include "app.h"
#include "app/color.h"
#include "app/color_utils.h"
#include "commands/commands.h"
#include "commands/params.h"
#include "core/cfg.h"
#include "document_wrappers.h"
#include "gui/gui.h"
#include "modules/editors.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "raster/raster.h"
#include "settings/settings.h"
#include "skin/skin_theme.h"
#include "tools/tool.h"
#include "ui_context.h"
#include "undo/undo_history.h"
#include "undoers/add_cel.h"
#include "undoers/add_image.h"
#include "undoers/close_group.h"
#include "undoers/dirty_area.h"
#include "undoers/open_group.h"
#include "undoers/replace_image.h"
#include "undoers/set_cel_position.h"
#include "util/boundary.h"
#include "util/misc.h"
#include "util/render.h"
#include "widgets/color_bar.h"
#include "widgets/editor/pixels_movement.h"
#include "widgets/statebar.h"

#include <allegro.h>
#include <stdio.h>

#define has_shifts(msg,shift)			\
  (((msg)->any.shifts & (shift)) == (shift))

#define has_only_shifts(msg,shift)		\
  (((msg)->any.shifts & (KB_SHIFT_FLAG |	\
		         KB_ALT_FLAG |		\
		         KB_CTRL_FLAG)) == (shift))

using namespace gfx;

static bool editor_view_msg_proc(JWidget widget, JMessage msg);

View* editor_view_new()
{
  View* widget = new View();
  SkinTheme* theme = static_cast<SkinTheme*>(widget->getTheme());
  int l = theme->get_part(PART_EDITOR_SELECTED_W)->w;
  int t = theme->get_part(PART_EDITOR_SELECTED_N)->h;
  int r = theme->get_part(PART_EDITOR_SELECTED_E)->w;
  int b = theme->get_part(PART_EDITOR_SELECTED_S)->h;

  jwidget_set_border(widget, l, t, r, b);
  widget->hideScrollBars();
  jwidget_add_hook(widget, JI_WIDGET, editor_view_msg_proc, NULL);

  return widget;
}

Editor::Editor()
  : Widget(editor_type())
{
  m_state = EDITOR_STATE_STANDBY;
  m_document = NULL;
  m_sprite = NULL;
  m_zoom = 0;

  m_cursor_thick = 0;
  m_cursor_screen_x = 0;
  m_cursor_screen_y = 0;
  m_cursor_editor_x = 0;
  m_cursor_editor_y = 0;

  m_cursor_candraw = false;
  m_insideSelection = false;

  m_quicktool = NULL;

  m_offset_x = 0;
  m_offset_y = 0;
  m_mask_timer_id = jmanager_add_timer(this, 100);
  m_offset_count = 0;

  m_refresh_region = NULL;
  m_toolLoopManager = NULL;
  m_pixelsMovement = NULL;

  jwidget_focusrest(this, true);

  m_currentToolChangeSlot =
    App::instance()->CurrentToolChange.connect(&Editor::onCurrentToolChange, this);
}

Editor::~Editor()
{
  jmanager_remove_timer(m_mask_timer_id);
  remove_editor(this);

  // Destroy tool-loop manager if it is created
  delete m_toolLoopManager;

  // Remove this editor as listener of CurrentToolChange signal.
  App::instance()->CurrentToolChange.disconnect(m_currentToolChangeSlot);
  delete m_currentToolChangeSlot;
}

int editor_type()
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

void Editor::setDocument(Document* document)
{
  if (this->hasMouse())
    jmanager_free_mouse();	// TODO Why is this here? Review this code

  // Do we need to drop files?
  if (m_pixelsMovement)
    dropPixels();

  ASSERT(m_pixelsMovement == NULL && "You cannot change the current sprite while you are moving pixels");
  ASSERT(m_state == EDITOR_STATE_STANDBY && "You can change the current sprite only in stand-by state");

  if (m_cursor_thick)
    editor_clean_cursor();

  // Change the sprite
  m_document = document;
  if (m_document) {
    m_sprite = m_document->getSprite();

    // Get the preferred doc's settings to edit it
    PreferredEditorSettings preferred = m_document->getPreferredEditorSettings();

    // Change the editor's configuration using the retrieved doc's settings
    m_zoom = preferred.zoom;

    updateEditor();

    if (preferred.virgin) {
      View* view = View::getView(this);
      Rect vp = view->getViewportBounds();

      preferred.virgin = false;
      preferred.scroll_x = -vp.w/2 + (m_sprite->getWidth()/2);
      preferred.scroll_y = -vp.h/2 + (m_sprite->getHeight()/2);

      m_document->setPreferredEditorSettings(preferred);
    }

    setEditorScroll(m_offset_x + preferred.scroll_x,
		    m_offset_y + preferred.scroll_y,
		    false);
  }
  // In this case document is NULL
  else {
    m_sprite = NULL;

    updateEditor();
    setEditorScroll(0, 0, false); // No scroll
  }

  // Redraw the entire editor (because we have a new sprite to draw)
  invalidate();
}

// Sets the scroll position of the editor
void Editor::setEditorScroll(int x, int y, int use_refresh_region)
{
  View* view = View::getView(this);
  Point oldScroll;
  JRegion region = NULL;
  int thick = m_cursor_thick;

  if (thick)
    editor_clean_cursor();

  if (use_refresh_region) {
    region = jwidget_get_drawable_region(this, JI_GDR_CUTTOPWINDOWS);
    oldScroll = view->getViewScroll();
  }

  view->setViewScroll(Point(x, y));
  Point newScroll = view->getViewScroll();

  if (m_document) {
    PreferredEditorSettings preferred;

    preferred.virgin = false;
    preferred.scroll_x = newScroll.x - m_offset_x;
    preferred.scroll_y = newScroll.y - m_offset_y;
    preferred.zoom = m_zoom;

    m_document->setPreferredEditorSettings(preferred);
  }

  if (use_refresh_region) {
    // Move screen with blits
    jwidget_scroll(this, region,
		   oldScroll.x - newScroll.x,
		   oldScroll.y - newScroll.y);
    
    jregion_free(region);
    /* m_widget->flags &= ~JI_DIRTY; */
  }
/*   else { */
/*     if (m_refresh_region) { */
/*       jregion_free (m_refresh_region); */
/*       m_refresh_region = NULL; */
/*     } */
/*   } */

  if (thick)
    editor_draw_cursor(m_cursor_screen_x, m_cursor_screen_y);
}

void Editor::updateEditor()
{
  View::getView(this)->updateView();
}

void Editor::drawSprite(int x1, int y1, int x2, int y2)
{
  View* view = View::getView(this);
  Rect vp = view->getViewportBounds();
  int source_x, source_y, dest_x, dest_y, width, height;

  // Get scroll

  Point scroll = view->getViewScroll();

  // Output information

  source_x = x1 << m_zoom;
  source_y = y1 << m_zoom;
  dest_x   = vp.x - scroll.x + m_offset_x + source_x;
  dest_y   = vp.y - scroll.y + m_offset_y + source_y;
  width    = (x2 - x1 + 1) << m_zoom;
  height   = (y2 - y1 + 1) << m_zoom;

  // Clip from viewport

  if (dest_x < vp.x) {
    source_x += vp.x - dest_x;
    width -= vp.x - dest_x;
    dest_x = vp.x;
  }

  if (dest_y < vp.y) {
    source_y += vp.y - dest_y;
    height -= vp.y - dest_y;
    dest_y = vp.y;
  }

  if (dest_x+width-1 > vp.x + vp.w-1)
    width = vp.x + vp.w - dest_x;

  if (dest_y+height-1 > vp.y + vp.h-1)
    height = vp.y + vp.h - dest_y;

  // Clip from screen

  if (dest_x < ji_screen->cl) {
    source_x += ji_screen->cl - dest_x;
    width -= ji_screen->cl - dest_x;
    dest_x = ji_screen->cl;
  }

  if (dest_y < ji_screen->ct) {
    source_y += ji_screen->ct - dest_y;
    height -= ji_screen->ct - dest_y;
    dest_y = ji_screen->ct;
  }

  if (dest_x+width-1 >= ji_screen->cr)
    width = ji_screen->cr-dest_x;

  if (dest_y+height-1 >= ji_screen->cb)
    height = ji_screen->cb-dest_y;

  // Clip from sprite

  if (source_x < 0) {
    width += source_x;
    dest_x -= source_x;
    source_x = 0;
  }

  if (source_y < 0) {
    height += source_y;
    dest_y -= source_y;
    source_y = 0;
  }

  if (source_x+width > (m_sprite->getWidth() << m_zoom)) {
    width = (m_sprite->getWidth() << m_zoom) - source_x;
  }

  if (source_y+height > (m_sprite->getHeight() << m_zoom)) {
    height = (m_sprite->getHeight() << m_zoom) - source_y;
  }

  // Draw the sprite

  if ((width > 0) && (height > 0)) {
    // Generate the rendered image
    Image* rendered = RenderEngine::renderSprite(m_document,
						 m_sprite,
						 source_x, source_y,
						 width, height,
						 m_sprite->getCurrentFrame(),
						 m_zoom, true);

    if (rendered) {
#ifdef DRAWSPRITE_DOUBLEBUFFERED
      BITMAP *bmp = create_bitmap(width, height);

      use_current_sprite_rgb_map();
      image_to_allegro(rendered, bmp, 0, 0, m_sprite->getCurrentPalette());
      blit(bmp, ji_screen, 0, 0, dest_x, dest_y, width, height);
      restore_rgb_map();

      image_free(rendered);
      destroy_bitmap(bmp);
#else
      acquire_bitmap(ji_screen);
      image_to_allegro(rendered, ji_screen, dest_x, dest_y, m_sprite->getCurrentPalette());
      release_bitmap(ji_screen);

      image_free(rendered);
#endif
    }
  }

  // Draw grids
  ISettings* settings = UIContext::instance()->getSettings();

  // Draw the pixel grid
  if (settings->getPixelGridVisible()) {
    if (m_zoom > 1)
      this->drawGrid(Rect(0, 0, 1, 1),
		     settings->getPixelGridColor());
  }

  // Draw the grid
  if (settings->getGridVisible())
    this->drawGrid(settings->getGridBounds(),
		   settings->getGridColor());
}

void Editor::drawSpriteSafe(int x1, int y1, int x2, int y2)
{
  JRegion region = jwidget_get_drawable_region(this, JI_GDR_CUTTOPWINDOWS);
  int c, nrects = JI_REGION_NUM_RECTS(region);
  int cx1, cy1, cx2, cy2;
  JRect rc;

  get_clip_rect(ji_screen, &cx1, &cy1, &cx2, &cy2);

  for (c=0, rc=JI_REGION_RECTS(region);
       c<nrects;
       c++, rc++) {
    add_clip_rect(ji_screen, rc->x1, rc->y1, rc->x2-1, rc->y2-1);
    drawSprite(x1, y1, x2, y2);
    set_clip_rect(ji_screen, cx1, cy1, cx2, cy2);
  }

  jregion_free(region);
}

/**
 * Draws the boundaries, really this routine doesn't use the "mask"
 * field of the sprite, only the "bound" field (so you can have other
 * mask in the sprite and could be showed other boundaries), to
 * regenerate boundaries, use the sprite_generate_mask_boundaries()
 * routine.
 */
void Editor::drawMask()
{
  View* view = View::getView(this);
  Rect vp = view->getViewportBounds();
  Point scroll = view->getViewScroll();
  int x1, y1, x2, y2;
  int c, x, y;

  dotted_mode(m_offset_count);

  x = vp.x - scroll.x + m_offset_x;
  y = vp.y - scroll.y + m_offset_y;

  int nseg = m_document->getBoundariesSegmentsCount();
  const _BoundSeg* seg = m_document->getBoundariesSegments();

  for (c=0; c<nseg; ++c, ++seg) {
    x1 = seg->x1<<m_zoom;
    y1 = seg->y1<<m_zoom;
    x2 = seg->x2<<m_zoom;
    y2 = seg->y2<<m_zoom;

#if 1				// Bounds inside mask
    if (!seg->open)
#else				// Bounds outside mask
    if (seg->open)
#endif
      {
	if (x1 == x2) {
	  x1--;
	  x2--;
	  y2--;
	}
	else {
	  y1--;
	  y2--;
	  x2--;
	}
      }
    else
      {
	if (x1 == x2) {
	  y2--;
	}
	else {
	  x2--;
	}
      }

    line(ji_screen, x+x1, y+y1, x+x2, y+y2, 0);
  }

  dotted_mode(-1);
}

void Editor::drawMaskSafe()
{
  if (m_document && m_document->getBoundariesSegments()) {
    int thick = m_cursor_thick;

    JRegion region = jwidget_get_drawable_region(this, JI_GDR_CUTTOPWINDOWS);
    int c, nrects = JI_REGION_NUM_RECTS(region);
    JRect rc;

    acquire_bitmap(ji_screen);

    if (thick)
      editor_clean_cursor();
    else
      jmouse_hide();

    for (c=0, rc=JI_REGION_RECTS(region);
	 c<nrects;
	 c++, rc++) {
      set_clip_rect(ji_screen, rc->x1, rc->y1, rc->x2-1, rc->y2-1);
      drawMask();
    }
    set_clip_rect(ji_screen, 0, 0, JI_SCREEN_W-1, JI_SCREEN_H-1);
    jregion_free(region);

    // Draw the cursor
    if (thick)
      editor_draw_cursor(m_cursor_screen_x, m_cursor_screen_y);
    else
      jmouse_show();

    release_bitmap(ji_screen);
  }
}

void Editor::drawGrid(const Rect& gridBounds, const Color& color)
{
  Rect grid(gridBounds);
  if (grid.w < 1 || grid.h < 1)
    return;

  int grid_color = color_utils::color_for_allegro(color, bitmap_color_depth(ji_screen));
  View* view = View::getView(this);
  Rect vp = view->getViewportBounds();
  Point scroll = view->getViewScroll();
  int x1, y1, x2, y2;
  int u1, v1, u2, v2;
  int c;

  scroll.x = vp.x - scroll.x + m_offset_x;
  scroll.y = vp.y - scroll.y + m_offset_y;

  x1 = ji_screen->cl;
  y1 = ji_screen->ct;
  x2 = ji_screen->cr-1;
  y2 = ji_screen->cb-1;

  screenToEditor(x1, y1, &u1, &v1);
  screenToEditor(x2, y2, &u2, &v2);

  grid.setOrigin(Point((grid.x % grid.w) - grid.w,
		       (grid.y % grid.h) - grid.h));

  u1 = ((u1-grid.x) / grid.w) - 1;
  v1 = ((v1-grid.y) / grid.h) - 1;
  u2 = ((u2-grid.x) / grid.w) + 1;
  v2 = ((v2-grid.y) / grid.h) + 1;

  grid.x <<= m_zoom;
  grid.y <<= m_zoom;
  grid.w <<= m_zoom;
  grid.h <<= m_zoom;

  // Horizontal lines
  x1 = scroll.x+grid.x+u1*grid.w;
  x2 = scroll.x+grid.x+u2*grid.w;

  for (c=v1; c<=v2; c++)
    hline(ji_screen, x1, scroll.y+grid.y+c*grid.h, x2, grid_color);

  // Vertical lines
  y1 = scroll.y+grid.y+v1*grid.h;
  y2 = scroll.y+grid.y+v2*grid.h;

  for (c=u1; c<=u2; c++)
    vline(ji_screen, scroll.x+grid.x+c*grid.w, y1, y2, grid_color);
}

void Editor::flashCurrentLayer()
{
  int x, y;
  const Image* src_image = m_sprite->getCurrentImage(&x, &y);
  if (src_image) {
    m_document->prepareExtraCel(0, 0, m_sprite->getWidth(), m_sprite->getHeight(), 255);
    Image* flash_image = m_document->getExtraCelImage();
    int u, v;

    image_clear(flash_image, flash_image->mask_color);
    for (v=0; v<flash_image->h; ++v) {
      for (u=0; u<flash_image->w; ++u) {
	if (u-x >= 0 && u-x < src_image->w &&
	    v-y >= 0 && v-y < src_image->h) {
	  uint32_t color = image_getpixel(src_image, u-x, v-y);
	  if (color != src_image->mask_color) {
	    Color ccc = Color::fromRgb(255, 255, 255);
	    image_putpixel(flash_image, u, v,
			   color_utils::color_for_image(ccc, flash_image->imgtype));
	  }
	}
      }
    }

    drawSprite(0, 0, m_sprite->getWidth()-1, m_sprite->getHeight()-1);
    gui_flip_screen();

    vsync();
    rest(100);

    image_clear(flash_image, flash_image->mask_color);
    drawSprite(0, 0, m_sprite->getWidth()-1, m_sprite->getHeight()-1);
  }
}

void Editor::setMaskColorForPixelsMovement(const Color& color)
{
  ASSERT(m_sprite != NULL);
  ASSERT(m_pixelsMovement != NULL);
  
  int imgtype = m_sprite->getImgType();
  m_pixelsMovement->setMaskColor(color_utils::color_for_image(color, imgtype));
}

/**
   Control scroll when cursor goes out of the editor
   
   @param msg A mouse message received in Editor::onProcessMessage()
*/
void Editor::controlInfiniteScroll(JMessage msg)
{
  View* view = View::getView(this);
  Rect vp = view->getViewportBounds();

  if (jmouse_control_infinite_scroll(vp)) {
    int old_x = msg->mouse.x;
    int old_y = msg->mouse.y;

    msg->mouse.x = jmouse_x(0);
    msg->mouse.y = jmouse_y(0);

    // Smooth scroll movement
    if (get_config_bool("Options", "MoveSmooth", TRUE)) {
      jmouse_set_position(MID(vp.x+1, old_x, vp.x+vp.w-2),
			  MID(vp.y+1, old_y, vp.y+vp.h-2));
    }
    // This is better for high resolutions: scroll movement by big steps
    else {
      jmouse_set_position((old_x != msg->mouse.x) ? (old_x + (vp.x+vp.w/2))/2: msg->mouse.x,
			  (old_y != msg->mouse.y) ? (old_y + (vp.y+vp.h/2))/2: msg->mouse.y);
    }

    msg->mouse.x = jmouse_x(0);
    msg->mouse.y = jmouse_y(0);

    Point scroll = view->getViewScroll();
    setEditorScroll(scroll.x+old_x-msg->mouse.x,
		    scroll.y+old_y-msg->mouse.y, true);
  }
}

void Editor::dropPixels()
{
  ASSERT(m_pixelsMovement != NULL);

  if (m_pixelsMovement->isDragging())
    m_pixelsMovement->dropImageTemporarily();

  // Drop pixels if the user press a button outside the selection
  m_pixelsMovement->dropImage();
  delete m_pixelsMovement;
  m_pixelsMovement = NULL;

  m_document->destroyExtraCel();

  m_state = EDITOR_STATE_STANDBY;
  releaseMouse();

  app_get_statusbar()->hideMovePixelsOptions();
  editor_update_statusbar_for_standby();
}

Tool* Editor::getCurrentEditorTool()
{
  if (m_quicktool)
    return m_quicktool;
  else {
    UIContext* context = UIContext::instance();
    return context->getSettings()->getCurrentTool();
  }
}

void Editor::screenToEditor(int xin, int yin, int *xout, int *yout)
{
  View* view = View::getView(this);
  Rect vp = view->getViewportBounds();
  Point scroll = view->getViewScroll();

  *xout = (xin - vp.x + scroll.x - m_offset_x) >> m_zoom;
  *yout = (yin - vp.y + scroll.y - m_offset_y) >> m_zoom;
}

void Editor::editorToScreen(int xin, int yin, int *xout, int *yout)
{
  View* view = View::getView(this);
  Rect vp = view->getViewportBounds();
  Point scroll = view->getViewScroll();

  *xout = (vp.x - scroll.x + m_offset_x + (xin << m_zoom));
  *yout = (vp.y - scroll.y + m_offset_y + (yin << m_zoom));
}

void Editor::showDrawingCursor()
{
  ASSERT(m_sprite != NULL);

  if (!m_cursor_thick && m_cursor_candraw) {
    jmouse_hide();
    editor_draw_cursor(jmouse_x(0), jmouse_y(0));
    jmouse_show();
  }
}

void Editor::hideDrawingCursor()
{
  if (m_cursor_thick) {
    jmouse_hide();
    editor_clean_cursor();
    jmouse_show();
  }
}

void Editor::editor_update_statusbar_for_standby()
{
  Tool* current_tool = getCurrentEditorTool();
  int x, y;
  screenToEditor(jmouse_x(0), jmouse_y(0), &x, &y);

  if (!m_sprite) {
    app_get_statusbar()->clearText();
  }
  // For eye-dropper
  else if (current_tool->getInk(0)->isEyedropper()) {
    int imgtype = m_sprite->getImgType();
    uint32_t pixel = m_sprite->getPixel(x, y);
    Color color = Color::fromImage(imgtype, pixel);

    int alpha = 255;
    switch (imgtype) {
      case IMAGE_RGB: alpha = _rgba_geta(pixel); break;
      case IMAGE_GRAYSCALE: alpha = _graya_geta(pixel); break;
    }

    char buf[256];
    usprintf(buf, "- Pos %d %d", x, y);

    app_get_statusbar()->showColor(0, buf, color, alpha);
  }
  // For other tools
  else {
    Mask* mask = m_document->getMask();

    app_get_statusbar()->setStatusText
      (0, "Pos %d %d, Size %d %d, Frame %d",
       x, y,
       ((mask && mask->bitmap)? mask->w: m_sprite->getWidth()),
       ((mask && mask->bitmap)? mask->h: m_sprite->getHeight()),
       m_sprite->getCurrentFrame()+1);
  }
}

// Update status bar for when the user is dragging pixels
void Editor::editor_update_statusbar_for_pixel_movement()
{
  ASSERT(m_pixelsMovement != NULL);

  Rect bounds = m_pixelsMovement->getImageBounds();
  app_get_statusbar()->setStatusText
    (100, "Pos %d %d, Size %d %d",
     bounds.x, bounds.y, bounds.w, bounds.h);
}

void Editor::editor_update_quicktool()
{
  Tool* old_quicktool = m_quicktool;

  m_quicktool = get_selected_quicktool();

  // If the tool has changed, we must to update the status bar because
  // the new tool can display something different in the status bar (e.g. Eyedropper)
  if (old_quicktool != m_quicktool)
    editor_update_statusbar_for_standby();
}

//////////////////////////////////////////////////////////////////////
// Message handler for the a view widget that contains an editor

static bool editor_view_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_SETPOS:
      // This avoid the displacement of the widgets in the viewport

      jrect_copy(widget->rc, &msg->setpos.rect);
      static_cast<View*>(widget)->updateView();
      return true;

    case JM_DRAW:
      {
	Widget* viewport = static_cast<View*>(widget)->getViewport();
	Widget* child = reinterpret_cast<JWidget>(jlist_first_data(viewport->children));
	JRect pos = jwidget_get_rect(widget);
	SkinTheme* theme = static_cast<SkinTheme*>(widget->getTheme());

	theme->draw_bounds_nw(ji_screen,
			      pos->x1, pos->y1,
			      pos->x2-1, pos->y2-1,
			      (child == current_editor) ? PART_EDITOR_SELECTED_NW:
							  PART_EDITOR_NORMAL_NW, false);

	jrect_free(pos);
      }
      return true;

  }
  return false;
}

//////////////////////////////////////////////////////////////////////
// Message handler for the editor

enum WHEEL_ACTION { WHEEL_NONE,
		    WHEEL_ZOOM,
		    WHEEL_VSCROLL,
		    WHEEL_HSCROLL,
		    WHEEL_FG,
		    WHEEL_BG,
		    WHEEL_FRAME };

bool Editor::onProcessMessage(JMessage msg)
{
  ASSERT((m_state == EDITOR_STATE_DRAWING && m_toolLoopManager != NULL) ||
	 (m_state != EDITOR_STATE_DRAWING && m_toolLoopManager == NULL));

  switch (msg->type) {

    case JM_REQSIZE:
      editor_request_size(&msg->reqsize.w, &msg->reqsize.h);
      return true;

    case JM_CLOSE:
      // if (m_refresh_region)
      // 	jregion_free(m_refresh_region);
      break;

    case JM_DRAW: {
      SkinTheme* theme = static_cast<SkinTheme*>(this->getTheme());

      int old_cursor_thick = m_cursor_thick;
      if (m_cursor_thick)
      	editor_clean_cursor();

      // Editor without sprite
      if (!m_sprite) {
	View* view = View::getView(this);
	Rect vp = view->getViewportBounds();

	jdraw_rectfill(vp, theme->get_editor_face_color());
	draw_emptyset_symbol(ji_screen, vp, makecol(64, 64, 64));
      }
      // Editor with sprite
      else {
	try {
	  // Lock the sprite to read/render it.
	  DocumentReader documentReader(m_document);
	  int x1, y1, x2, y2;

	  // Draw the background outside of sprite's bounds
	  x1 = this->rc->x1 + m_offset_x;
	  y1 = this->rc->y1 + m_offset_y;
	  x2 = x1 + (m_sprite->getWidth() << m_zoom) - 1;
	  y2 = y1 + (m_sprite->getHeight() << m_zoom) - 1;

	  jrectexclude(ji_screen,
		       this->rc->x1, this->rc->y1,
		       this->rc->x2-1, this->rc->y2-1,
		       x1-1, y1-1, x2+1, y2+2, theme->get_editor_face_color());

	  // Draw the sprite in the editor
	  drawSprite(0, 0, m_sprite->getWidth()-1, m_sprite->getHeight()-1);

	  // Draw the sprite boundary
	  rect(ji_screen, x1-1, y1-1, x2+1, y2+1, theme->get_editor_sprite_border());
	  hline(ji_screen, x1-1, y2+2, x2+1, theme->get_editor_sprite_bottom_edge());

	  // Draw the mask boundaries
	  if (m_document->getBoundariesSegments()) {
	    drawMask();
	    jmanager_start_timer(m_mask_timer_id);
	  }
	  else {
	    jmanager_stop_timer(m_mask_timer_id);
	  }

	  if (old_cursor_thick != 0) {
	    editor_draw_cursor(jmouse_x(0), jmouse_y(0));
	  }
	}
	catch (const LockedDocumentException&) {
	  // The sprite is locked to be read, so we can draw an opaque
	  // background only.

	  View* view = View::getView(this);
	  Rect vp = view->getViewportBounds();
	  jdraw_rectfill(vp, theme->get_editor_face_color());
	}
      }

      return true;
    }

    case JM_TIMER:
      if (msg->timer.timer_id == m_mask_timer_id) {
	if (m_sprite) {
	  drawMaskSafe();

	  // Set offset to make selection-movement effect
	  if (m_offset_count < 7)
	    m_offset_count++;
	  else
	    m_offset_count = 0;
	}
      }
      break;

    case JM_MOUSEENTER:
      // When the mouse enter to the editor, we can calculate the
      // 'cursor_candraw' field to avoid a heavy if-condition in the
      // 'editor_setcursor' routine
      editor_update_candraw();
      editor_update_quicktool();
      break;

    case JM_MOUSELEAVE:
      hideDrawingCursor();
      app_get_statusbar()->clearText();
      break;

    case JM_BUTTONPRESSED:
      if (!m_sprite)
	break;

      // Drawing loop
      if (m_state == EDITOR_STATE_DRAWING) {
	ASSERT(m_toolLoopManager != NULL);

	m_toolLoopManager->pressButton(msg);

	// Cancel drawing loop
	if (m_toolLoopManager->isCanceled()) {
	  m_toolLoopManager->releaseLoop(msg);

	  delete m_toolLoopManager;
	  m_toolLoopManager = NULL;
	  m_state = EDITOR_STATE_STANDBY;

	  // Redraw all the editors with this sprite
	  update_screen_for_document(m_document);

	  clear_keybuf();

	  editor_setcursor(msg->mouse.x, msg->mouse.y);

	  releaseMouse();
	  return true;
	}
      }

      if (!hasCapture()) {
	UIContext* context = UIContext::instance();
	Tool* current_tool = getCurrentEditorTool();

	set_current_editor(this);
	context->setActiveDocument(m_document);

	// Start scroll loop
	if (msg->mouse.middle ||
	    current_tool->getInk(msg->mouse.right ? 1: 0)->isScrollMovement()) {
	  m_state = EDITOR_STATE_SCROLLING;

	  editor_setcursor(msg->mouse.x, msg->mouse.y);
	  captureMouse();
	  return true;
	}

	if (m_pixelsMovement) {
	  // Start "moving pixels" loop
	  if (m_insideSelection) {
	    // Re-catch the image
	    int x, y;
	    screenToEditor(msg->mouse.x, msg->mouse.y, &x, &y);
	    m_pixelsMovement->catchImageAgain(x, y);

	    captureMouse();
	    return true;
	  }
	  // End "moving pixels" loop
	  else {
	    // Drop pixels (e.g. to start drawing)
	    dropPixels();
	  }
	}

	// Move frames position
	if (current_tool->getInk(msg->mouse.right ? 1: 0)->isCelMovement()) {
	  if ((m_sprite->getCurrentLayer()) &&
	      (m_sprite->getCurrentLayer()->getType() == GFXOBJ_LAYER_IMAGE)) {
	    // TODO you can move the `Background' with tiled mode
	    if (m_sprite->getCurrentLayer()->is_background()) {
	      Alert::show(PACKAGE
			  "<<You can't move the `Background' layer."
			  "||&Close");
	    }
	    else if (!m_sprite->getCurrentLayer()->is_moveable()) {
	      Alert::show(PACKAGE "<<The layer movement is locked.||&Close");
	    }
	    else {
	      bool click2 = get_config_bool("Options", "MoveClick2", FALSE);
	      interactive_move_layer(click2 ? MODE_CLICKANDCLICK:
					      MODE_CLICKANDRELEASE,
				     TRUE, NULL); // TODO remove this routine
	    }
	  }
	}
	// Move selected pixels
	else if (m_insideSelection &&
		 current_tool->getInk(0)->isSelection() &&
		 msg->mouse.left) {
	  int x, y, opacity;
	  Image* image = m_sprite->getCurrentImage(&x, &y, &opacity);
	  if (image) {
	    if (!m_sprite->getCurrentLayer()->is_writable()) {
	      Alert::show(PACKAGE "<<The layer is locked.||&Close");
	      return true;
	    }

	    // Copy the mask to the extra cel image
	    Image* tmpImage = NewImageFromMask(m_document);
	    x = m_document->getMask()->x;
	    y = m_document->getMask()->y;
	    m_pixelsMovement = new PixelsMovement(m_document, m_sprite, tmpImage, x, y, opacity);
	    delete tmpImage;

	    // If the CTRL key is pressed start dragging a copy of the selection
	    if (key[KEY_LCONTROL] || key[KEY_RCONTROL]) // TODO configurable
	      m_pixelsMovement->copyMask();
	    else
	      m_pixelsMovement->cutMask();

	    screenToEditor(msg->mouse.x, msg->mouse.y, &x, &y);
	    m_pixelsMovement->catchImage(x, y);

	    // Setup mask color
	    setMaskColorForPixelsMovement(app_get_statusbar()->getTransparentColor());

	    // Update status bar
	    editor_update_statusbar_for_pixel_movement();
	    app_get_statusbar()->showMovePixelsOptions();
	  }
	  captureMouse();
	}
	// Call the eyedropper command
	else if (current_tool->getInk(msg->mouse.right ? 1: 0)->isEyedropper()) {
	  Command* eyedropper_cmd = 
	    CommandsModule::instance()->getCommandByName(CommandId::Eyedropper);

	  Params params;
	  params.set("target", msg->mouse.right ? "background": "foreground");

	  UIContext::instance()->executeCommand(eyedropper_cmd, &params);
	  return true;
	}
	// Start the Tool-Loop
	else if (m_sprite->getCurrentLayer()) {
	  ASSERT(m_toolLoopManager == NULL);

	  IToolLoop* toolLoop = createToolLoopImpl(UIContext::instance(), msg);
	  if (!toolLoop)
	    return true;	// Return without capturing mouse

	  m_toolLoopManager = new ToolLoopManager(toolLoop);
	  if (!m_toolLoopManager)
	    return true;	// Return without capturing mouse

	  // Clean the cursor (mainly to clean the pen preview)
	  int thick = m_cursor_thick;
	  if (thick)
	    editor_clean_cursor();

	  m_state = EDITOR_STATE_DRAWING;

	  m_toolLoopManager->prepareLoop(msg);
	  m_toolLoopManager->pressButton(msg);

	  // Redraw it (without pen preview)
	  if (thick)
	    editor_draw_cursor(msg->mouse.x, msg->mouse.y);

	  captureMouse();
	}
      }
      return true;

    case JM_MOTION:
      if (!m_sprite)
	break;

      // Move the scroll
      if (m_state == EDITOR_STATE_SCROLLING) {
	View* view = View::getView(this);
	Rect vp = view->getViewportBounds();
	Point scroll = view->getViewScroll();

	setEditorScroll(scroll.x+jmouse_x(1)-jmouse_x(0),
			scroll.y+jmouse_y(1)-jmouse_y(0), true);

	jmouse_control_infinite_scroll(vp);

	{
	  int x, y;
	  screenToEditor(jmouse_x(0), jmouse_y(0), &x, &y);
	  app_get_statusbar()->setStatusText
	    (0, "Pos %3d %3d (Size %3d %3d)", x, y,
	     m_sprite->getWidth(), m_sprite->getHeight());
	}
      }
      // Moving pixels
      else if (m_pixelsMovement) {
	// If there is a button pressed
	if (m_pixelsMovement->isDragging()) {
	  // Infinite scroll
	  controlInfiniteScroll(msg);

	  // Get the position of the mouse in the sprite 
	  int x, y;
	  screenToEditor(msg->mouse.x, msg->mouse.y, &x, &y);

	  // Drag the image to that position
	  Rect bounds = m_pixelsMovement->moveImage(x, y);

	  // If "bounds" is empty is because the cel was not moved
	  if (!bounds.isEmpty()) {
	    // Redraw the extra cel in the new position
	    jmouse_hide();
	    editors_draw_sprite_tiled(m_sprite,
				      bounds.x, bounds.y,
				      bounds.x+bounds.w-1,
				      bounds.y+bounds.h-1);
	    jmouse_show();
	  }
	}
	else {
	  // Draw cursor
	  if (m_cursor_thick) {
	    int x, y;

	    x = msg->mouse.x;
	    y = msg->mouse.y;

	    // Redraw it only when the mouse change to other pixel (not
	    // when the mouse moves only).
	    if ((m_cursor_screen_x != x) || (m_cursor_screen_y != y)) {
	      jmouse_hide();
	      editor_move_cursor(x, y);
	      jmouse_show();
	    }
	  }
	}

	editor_update_statusbar_for_pixel_movement();
      }
      // In tool-loop
      else if (m_state == EDITOR_STATE_DRAWING) {
	acquire_bitmap(ji_screen);

	ASSERT(m_toolLoopManager != NULL);

	// Clean the area occupied by the cursor in the screen
	if (m_cursor_thick)
	  editor_clean_cursor();

	ASSERT(m_toolLoopManager != NULL);

	// Infinite scroll
	controlInfiniteScroll(msg);

	// Clean the area occupied by the cursor in the screen
	int thick = m_cursor_thick; 
	if (thick)
	  editor_clean_cursor();

	// notify mouse movement to the tool
	ASSERT(m_toolLoopManager != NULL);
	m_toolLoopManager->movement(msg);

	// draw the cursor again
	if (thick)
	  editor_draw_cursor(msg->mouse.x, msg->mouse.y);

	release_bitmap(ji_screen);
      }
      else if (m_state == EDITOR_STATE_STANDBY) {
	// Draw cursor
	if (m_cursor_thick) {
	  int x, y;

	  x = msg->mouse.x;
	  y = msg->mouse.y;

	  // Redraw it only when the mouse change to other pixel (not
	  // when the mouse moves only).
	  if ((m_cursor_screen_x != x) || (m_cursor_screen_y != y)) {
	    jmouse_hide();
	    editor_move_cursor(x, y);
	    jmouse_show();
	  }
	}

	editor_update_statusbar_for_standby();
      }
      return true;

    case JM_BUTTONRELEASED:
      if (!m_sprite)
	break;

      // Drawing
      if (m_state == EDITOR_STATE_DRAWING) {
	ASSERT(m_toolLoopManager != NULL);

	if (m_toolLoopManager->releaseButton(msg))
	  return true;

	m_toolLoopManager->releaseLoop(msg);

	delete m_toolLoopManager;
	m_toolLoopManager = NULL;
	m_state = EDITOR_STATE_STANDBY;

	// redraw all the editors with this sprite
	update_screen_for_document(m_document);

	clear_keybuf();
      }
      else if (m_state != EDITOR_STATE_STANDBY) {
	ASSERT(m_toolLoopManager == NULL);
	m_state = EDITOR_STATE_STANDBY;
      }
      // Moving pixels
      else if (m_pixelsMovement) {
	// Drop the image temporarily in this location (where the user releases the mouse)
	m_pixelsMovement->dropImageTemporarily();
      }

      editor_setcursor(msg->mouse.x, msg->mouse.y);
      editor_update_statusbar_for_standby();
      releaseMouse();
      return true;

    case JM_KEYPRESSED:
      if (m_state == EDITOR_STATE_STANDBY ||
	  m_state == EDITOR_STATE_DRAWING) {
	if (editor_keys_toset_zoom(msg->key.scancode))
	  return true;
      }

      if (this->hasMouse()) {
	editor_update_quicktool();

	if (msg->key.scancode == KEY_LCONTROL || // TODO configurable
	    msg->key.scancode == KEY_RCONTROL) {
	  // If the user press the CTRL key when he is dragging pixels (but not pressing the mouse buttons)...
	  if (!jmouse_b(0) && m_pixelsMovement) {
	    // Drop pixels (sure the user will press the mouse button to start dragging a copy)
	    dropPixels();
	  }
	}

	editor_setcursor(jmouse_x(0), jmouse_y(0));
      }

      // When we are drawing, we "eat" all pressed keys
      if (m_state == EDITOR_STATE_DRAWING)
	return true;

      break;

    case JM_KEYRELEASED:
      editor_update_quicktool();
      editor_setcursor(jmouse_x(0), jmouse_y(0));
      break;

    case JM_FOCUSLEAVE:
      // As we use keys like Space-bar as modifier, we can clear the
      // keyboard buffer when we lost the focus.
      clear_keybuf();
      break;

    case JM_WHEEL:
      if (m_state == EDITOR_STATE_STANDBY ||
	  m_state == EDITOR_STATE_DRAWING) {
	// There are and sprite in the editor and the mouse is inside
	if (m_sprite && this->hasMouse()) {
	  int dz = jmouse_z(1) - jmouse_z(0);
	  WHEEL_ACTION wheelAction = WHEEL_NONE;
	  bool scrollBigSteps = false;

	  // Without modifiers
	  if (!(msg->any.shifts & (KB_SHIFT_FLAG | KB_ALT_FLAG | KB_CTRL_FLAG))) {
	    wheelAction = WHEEL_ZOOM;
	  }
	  else {
#if 1				// TODO make it configurable
	    if (has_shifts(msg, KB_ALT_FLAG)) {
	      if (has_shifts(msg, KB_SHIFT_FLAG))
		wheelAction = WHEEL_BG;
	      else
		wheelAction = WHEEL_FG;
	    }
	    else if (has_shifts(msg, KB_CTRL_FLAG)) {
	      wheelAction = WHEEL_FRAME;
	    }
#else
	    if (has_shifts(msg, KB_CTRL_FLAG))
	      wheelAction = WHEEL_HSCROLL;
	    else
	      wheelAction = WHEEL_VSCROLL;

	    if (has_shifts(msg, KB_SHIFT_FLAG))
	      scrollBigSteps = true;
#endif
	  }

	  switch (wheelAction) {

	    case WHEEL_NONE:
	      // Do nothing
	      break;

	    case WHEEL_FG:
	      if (m_state == EDITOR_STATE_STANDBY) {
		int newIndex = 0;
		if (app_get_colorbar()->getFgColor().getType() == Color::IndexType) {
		  newIndex = app_get_colorbar()->getFgColor().getIndex() + dz;
		  newIndex = MID(0, newIndex, 255);
		}
		app_get_colorbar()->setFgColor(Color::fromIndex(newIndex));
	      }
	      break;

	    case WHEEL_BG:
	      if (m_state == EDITOR_STATE_STANDBY) {
		int newIndex = 0;
		if (app_get_colorbar()->getBgColor().getType() == Color::IndexType) {
		  newIndex = app_get_colorbar()->getBgColor().getIndex() + dz;
		  newIndex = MID(0, newIndex, 255);
		}
		app_get_colorbar()->setBgColor(Color::fromIndex(newIndex));
	      }
	      break;

	    case WHEEL_FRAME:
	      if (m_state == EDITOR_STATE_STANDBY) {
		Command* command = CommandsModule::instance()->getCommandByName
		  ((dz < 0) ? CommandId::GotoNextFrame:
			      CommandId::GotoPreviousFrame);
		if (command)
		  UIContext::instance()->executeCommand(command, NULL);
	      }
	      break;

	    case WHEEL_ZOOM: {
	      int zoom = MID(MIN_ZOOM, m_zoom-dz, MAX_ZOOM);
	      if (m_zoom != zoom)
		setZoomAndCenterInMouse(zoom, msg->mouse.x, msg->mouse.y);
	      break;
	    }

	    case WHEEL_HSCROLL:
	    case WHEEL_VSCROLL: {
	      View* view = View::getView(this);
	      Rect vp = view->getViewportBounds();
	      Point scroll;
	      int dx = 0;
	      int dy = 0;
	      int thick = m_cursor_thick;

	      if (wheelAction == WHEEL_HSCROLL) {
		dx = dz * vp.w;
	      }
	      else {
		dy = dz * vp.h;
	      }

	      if (scrollBigSteps) {
		dx /= 2;
		dy /= 2;
	      }
	      else {
		dx /= 10;
		dy /= 10;
	      }
		
	      scroll = view->getViewScroll();

	      jmouse_hide();
	      if (thick)
		editor_clean_cursor();
	      setEditorScroll(scroll.x+dx, scroll.y+dy, true);
	      if (thick)
		editor_draw_cursor(jmouse_x(0), jmouse_y(0));
	      jmouse_show();
	      break;
	    }

	  }
	}
      }
      break;

    case JM_SETCURSOR:
      editor_setcursor(msg->mouse.x, msg->mouse.y);
      return true;

  }

  return Widget::onProcessMessage(msg);
}

// When the current tool is changed
void Editor::onCurrentToolChange()
{
  Tool* current_tool = getCurrentEditorTool();

  // If the user changed the tool when he/she is moving pixels,
  // we have to drop the pixels only if the new tool is not selection...
  if (m_pixelsMovement &&
      (!current_tool->getInk(0)->isSelection() ||
       !current_tool->getInk(1)->isSelection())) {
    // We have to drop pixels
    dropPixels();
  }
}

/**
 * Returns size for the editor viewport
 */
void Editor::editor_request_size(int *w, int *h)
{
  if (m_sprite) {
    View* view = View::getView(this);
    Rect vp = view->getViewportBounds();

    m_offset_x = vp.w/2 - 1;
    m_offset_y = vp.h/2 - 1;

    *w = (m_sprite->getWidth() << m_zoom) + m_offset_x*2;
    *h = (m_sprite->getHeight() << m_zoom) + m_offset_y*2;
  }
  else {
    *w = 4;
    *h = 4;
  }
}

void Editor::editor_setcursor(int x, int y)
{
  Tool* current_tool = getCurrentEditorTool();

  switch (m_state) {

    case EDITOR_STATE_SCROLLING:
      hideDrawingCursor();
      jmouse_set_cursor(JI_CURSOR_SCROLL);
      break;

    case EDITOR_STATE_DRAWING:
      if (current_tool->getInk(0)->isEyedropper()) {
	hideDrawingCursor();
	jmouse_set_cursor(JI_CURSOR_EYEDROPPER);
	return;
      }
      else {
	jmouse_set_cursor(JI_CURSOR_NULL);
	showDrawingCursor();
      }
      break;

    case EDITOR_STATE_STANDBY:
      if (m_sprite) {
	Tool* current_tool = getCurrentEditorTool();

	editor_update_candraw(); // TODO remove this

	// Pixels movement
	if (m_pixelsMovement) {
	  int x, y;
	  screenToEditor(jmouse_x(0), jmouse_y(0), &x, &y);
	    
	  // Move selection
	  if (m_pixelsMovement->isDragging() ||
	      m_document->isMaskVisible() &&
	      m_document->getMask()->contains_point(x, y)) {
	    hideDrawingCursor();
	    jmouse_set_cursor(JI_CURSOR_MOVE);

	    if (!m_insideSelection)
	      m_insideSelection = true;
	    return;
	  }

	  if (m_insideSelection)
	    m_insideSelection = false;

	  // Draw
	  if (m_cursor_candraw) {
	    jmouse_set_cursor(JI_CURSOR_NULL);
	    showDrawingCursor();
	  }
	  // Forbidden
	  else {
	    hideDrawingCursor();
	    jmouse_set_cursor(JI_CURSOR_FORBIDDEN);
	  }
	}
	else {
	  if (current_tool) {
	    // If the current tool change selection (e.g. rectangular marquee, etc.)
	    if (current_tool->getInk(0)->isSelection()) {
	      int x, y;
	      screenToEditor(jmouse_x(0), jmouse_y(0), &x, &y);
	    
	      // Move pixels
	      if (m_document->isMaskVisible() &&
		  m_document->getMask()->contains_point(x, y)) {
		hideDrawingCursor();
		if (key[KEY_LCONTROL] ||
		    key[KEY_RCONTROL]) // TODO configurable keys
		  jmouse_set_cursor(JI_CURSOR_NORMAL_ADD);
		else
		  jmouse_set_cursor(JI_CURSOR_MOVE);

		if (!m_insideSelection)
		  m_insideSelection = true;
		return;
	      }
	    }
	    else if (current_tool->getInk(0)->isEyedropper()) {
	      hideDrawingCursor();
	      jmouse_set_cursor(JI_CURSOR_EYEDROPPER);
	      return;
	    }
	    else if (current_tool->getInk(0)->isScrollMovement()) {
	      hideDrawingCursor();
	      jmouse_set_cursor(JI_CURSOR_SCROLL);
	      return;
	    }
	    else if (current_tool->getInk(0)->isCelMovement()) {
	      hideDrawingCursor();
	      jmouse_set_cursor(JI_CURSOR_MOVE);
	      return;
	    }
	  }

	  if (m_insideSelection)
	    m_insideSelection = false;

	  // Draw
	  if (m_cursor_candraw) {
	    jmouse_set_cursor(JI_CURSOR_NULL);
	    showDrawingCursor();
	  }
	  // Forbidden
	  else {
	    hideDrawingCursor();
	    jmouse_set_cursor(JI_CURSOR_FORBIDDEN);
	  }
	}
      }
      else {
	hideDrawingCursor();
	jmouse_set_cursor(JI_CURSOR_NORMAL);
      }
      break;

  }
}

/* TODO this routine should be called in a change of context */
void Editor::editor_update_candraw()
{
  m_cursor_candraw =
    (m_sprite != NULL &&
     m_sprite->getCurrentLayer() != NULL &&
     m_sprite->getCurrentLayer()->is_image() &&
     m_sprite->getCurrentLayer()->is_readable() &&
     m_sprite->getCurrentLayer()->is_writable() /* && */
     /* layer_get_cel(m_sprite->layer, m_sprite->frame) != NULL */
     );
}

void Editor::setZoomAndCenterInMouse(int zoom, int mouse_x, int mouse_y)
{
  View* view = View::getView(this);
  Rect vp = view->getViewportBounds();
  int x, y;
  bool centerMouse = get_config_bool("Editor", "CenterMouseInZoom", false);
  int mx, my;

  hideDrawingCursor();
  screenToEditor(mouse_x, mouse_y, &x, &y);

  if (centerMouse) {
    mx = vp.x+vp.w/2;
    my = vp.y+vp.h/2;
  }
  else {
    mx = mouse_x;
    my = mouse_y;
  }

  x = m_offset_x - (mx - vp.x) + ((1<<zoom)>>1) + (x << zoom);
  y = m_offset_y - (my - vp.y) + ((1<<zoom)>>1) + (y << zoom);

  if ((m_zoom != zoom) ||
      (m_cursor_editor_x != mx) ||
      (m_cursor_editor_y != my)) {
    int use_refresh_region = (m_zoom == zoom) ? true: false;

    m_zoom = zoom;

    updateEditor();
    setEditorScroll(x, y, use_refresh_region);

    if (centerMouse)
      jmouse_set_position(mx, my);
  }
  showDrawingCursor();
}

//////////////////////////////////////////////////////////////////////
// IToolLoop implementation

class ToolLoopImpl : public IToolLoop
{
  Editor* m_editor;
  Context* m_context;
  Tool* m_tool;
  Pen* m_pen;
  Document* m_document;
  Sprite* m_sprite;
  Layer* m_layer;
  Cel* m_cel;
  Image* m_cel_image;
  bool m_cel_created;
  int m_old_cel_x;
  int m_old_cel_y;
  bool m_filled;
  bool m_previewFilled;
  int m_sprayWidth;
  int m_spraySpeed;
  TiledMode m_tiled_mode;
  Image* m_src_image;
  Image* m_dst_image;
  bool m_useMask;
  Mask* m_mask;
  Point m_maskOrigin;
  int m_opacity;
  int m_tolerance;
  Point m_offset;
  Point m_speed;
  bool m_canceled;
  int m_button;
  int m_primary_color;
  int m_secondary_color;

public:
  ToolLoopImpl(Editor* editor,
	       Context* context,
	       Tool* tool,
	       Document* document,
	       Sprite* sprite,
	       Layer* layer,
	       int button, const Color& primary_color, const Color& secondary_color)
    : m_editor(editor)
    , m_context(context)
    , m_tool(tool)
    , m_document(document)
    , m_sprite(sprite)
    , m_layer(layer)
    , m_cel(NULL)
    , m_cel_image(NULL)
    , m_cel_created(false)
    , m_canceled(false)
    , m_button(button)
    , m_primary_color(color_utils::color_for_layer(primary_color, layer))
    , m_secondary_color(color_utils::color_for_layer(secondary_color, layer))
  {
    // Settings
    ISettings* settings = m_context->getSettings();

    m_tiled_mode = settings->getTiledMode();

    switch (tool->getFill(m_button)) {
      case TOOL_FILL_NONE:
	m_filled = false;
	break;
      case TOOL_FILL_ALWAYS:
	m_filled = true;
	break;
      case TOOL_FILL_OPTIONAL:
	m_filled = settings->getToolSettings(m_tool)->getFilled();
	break;
    }
    m_previewFilled = settings->getToolSettings(m_tool)->getPreviewFilled();

    m_sprayWidth = settings->getToolSettings(m_tool)->getSprayWidth();
    m_spraySpeed = settings->getToolSettings(m_tool)->getSpraySpeed();

    // Create the pen
    IPenSettings* pen_settings = settings->getToolSettings(m_tool)->getPen();
    ASSERT(pen_settings != NULL);

    m_pen = new Pen(pen_settings->getType(),
		    pen_settings->getSize(),
		    pen_settings->getAngle());
    
    // Get cel and image where we can draw

    if (m_layer->is_image()) {
      m_cel = static_cast<LayerImage*>(sprite->getCurrentLayer())->getCel(sprite->getCurrentFrame());
      if (m_cel)
	m_cel_image = sprite->getStock()->getImage(m_cel->getImage());
    }

    if (m_cel == NULL) {
      // create the image
      m_cel_image = image_new(sprite->getImgType(), sprite->getWidth(), sprite->getHeight());
      image_clear(m_cel_image,
		  m_cel_image->mask_color);

      // create the cel
      m_cel = new Cel(sprite->getCurrentFrame(), 0);
      static_cast<LayerImage*>(sprite->getCurrentLayer())->addCel(m_cel);

      m_cel_created = true;
    }

    m_old_cel_x = m_cel->getX();
    m_old_cel_y = m_cel->getY();

    // region to draw
    int x1, y1, x2, y2;

    // non-tiled
    if (m_tiled_mode == TILED_NONE) {
      x1 = MIN(m_cel->getX(), 0);
      y1 = MIN(m_cel->getY(), 0);
      x2 = MAX(m_cel->getX()+m_cel_image->w, m_sprite->getWidth());
      y2 = MAX(m_cel->getY()+m_cel_image->h, m_sprite->getHeight());
    }
    else { 			// tiled
      x1 = 0;
      y1 = 0;
      x2 = m_sprite->getWidth();
      y2 = m_sprite->getHeight();
    }

    // create two copies of the image region which we'll modify with the tool
    m_src_image = image_crop(m_cel_image,
			     x1-m_cel->getX(),
			     y1-m_cel->getY(), x2-x1, y2-y1,
			     m_cel_image->mask_color);
    m_dst_image = image_new_copy(m_src_image);

    m_useMask = m_document->isMaskVisible();

    // Selection ink
    if (getInk()->isSelection() && !m_document->isMaskVisible()) {
      Mask emptyMask;
      m_document->setMask(&emptyMask);
    }

    m_mask = m_document->getMask();
    m_maskOrigin = (!m_mask->is_empty() ? Point(m_mask->x-x1, m_mask->y-y1):
					  Point(0, 0));

    m_opacity = settings->getToolSettings(m_tool)->getOpacity();
    m_tolerance = settings->getToolSettings(m_tool)->getTolerance();
    m_speed.x = 0;
    m_speed.y = 0;

    // we have to modify the cel position because it's used in the
    // `render_sprite' routine to draw the `dst_image'
    m_cel->setPosition(x1, y1);
    m_offset.x = -x1;
    m_offset.y = -y1;

    // Set undo label for any kind of undo used in the whole loop
    if (m_document->getUndoHistory()->isEnabled()) {
      m_document->getUndoHistory()->setLabel(m_tool->getText().c_str());

      if (getInk()->isSelection() ||
	  getInk()->isEyedropper() ||
	  getInk()->isScrollMovement()) {
	m_document->getUndoHistory()->setModification(undo::DoesntModifyDocument);
      }
      else
	m_document->getUndoHistory()->setModification(undo::ModifyDocument);
    }
  }

  ~ToolLoopImpl()
  {
    if (!m_canceled) {
      undo::UndoHistory* undo = m_document->getUndoHistory();

      // Paint ink
      if (getInk()->isPaint()) {
	// If the size of each image is the same, we can create an
	// undo with only the differences between both images.
	if (m_cel->getX() == m_old_cel_x &&
	    m_cel->getY() == m_old_cel_y &&
	    m_cel_image->w == m_dst_image->w &&
	    m_cel_image->h == m_dst_image->h) {
	  // Was the 'cel_image' created in the start of the tool-loop?.
	  if (m_cel_created) {
	    // Then we can keep the 'cel_image'...
	  
	    // We copy the 'destination' image to the 'cel_image'.
	    image_copy(m_cel_image, m_dst_image, 0, 0);

	    // Add the 'cel_image' in the images' stock of the sprite.
	    m_cel->setImage(m_sprite->getStock()->addImage(m_cel_image));

	    // Is the undo enabled?.
	    if (undo->isEnabled()) {
	      // We can temporary remove the cel.
	      static_cast<LayerImage*>(m_sprite->getCurrentLayer())->removeCel(m_cel);

	      // We create the undo information (for the new cel_image
	      // in the stock and the new cel in the layer)...
	      undo->pushUndoer(new undoers::OpenGroup());
	      undo->pushUndoer(new undoers::AddImage(undo->getObjects(),
		  m_sprite->getStock(), m_cel->getImage()));
	      undo->pushUndoer(new undoers::AddCel(undo->getObjects(),
		  m_sprite->getCurrentLayer(), m_cel));
	      undo->pushUndoer(new undoers::CloseGroup());

	      // And finally we add the cel again in the layer.
	      static_cast<LayerImage*>(m_sprite->getCurrentLayer())->addCel(m_cel);
	    }
	  }
	  else {
	    // Undo the dirty region.
	    if (undo->isEnabled()) {
	      Dirty* dirty = new Dirty(m_cel_image, m_dst_image);
	      // TODO error handling

	      dirty->saveImagePixels(m_cel_image);
	      if (dirty != NULL)
		undo->pushUndoer(new undoers::DirtyArea(undo->getObjects(),
		    m_cel_image, dirty));

	      delete dirty;
	    }

	    // Copy the 'dst_image' to the cel_image.
	    image_copy(m_cel_image, m_dst_image, 0, 0);
	  }
	}
	// If the size of both images are different, we have to
	// replace the entire image.
	else {
	  if (undo->isEnabled()) {
	    undo->pushUndoer(new undoers::OpenGroup());

	    if (m_cel->getX() != m_old_cel_x ||
		m_cel->getY() != m_old_cel_y) {
	      int x = m_cel->getX();
	      int y = m_cel->getY();
	      m_cel->setPosition(m_old_cel_x, m_old_cel_y);

	      undo->pushUndoer(new undoers::SetCelPosition(undo->getObjects(), m_cel));

	      m_cel->setPosition(x, y);
	    }

	    undo->pushUndoer(new undoers::ReplaceImage(undo->getObjects(),
		m_sprite->getStock(), m_cel->getImage()));
	    undo->pushUndoer(new undoers::CloseGroup());
	  }

	  // Replace the image in the stock.
	  m_sprite->getStock()->replaceImage(m_cel->getImage(), m_dst_image);

	  // Destroy the old cel image.
	  image_free(m_cel_image);

	  // Now the `dst_image' is used, so we haven't to destroy it.
	  m_dst_image = NULL;
	}
      }

      // Selection ink
      if (getInk()->isSelection())
	m_document->generateMaskBoundaries();
    }

    // If the trace was not canceled or it is not a 'paint' ink...
    if (m_canceled || !getInk()->isPaint()) {
      // Here we destroy the temporary 'cel' created and restore all as it was before

      m_cel->setPosition(m_old_cel_x, m_old_cel_y);

      if (m_cel_created) {
	static_cast<LayerImage*>(m_layer)->removeCel(m_cel);
	delete m_cel;
	delete m_cel_image;
      }
    }

    delete m_src_image;
    delete m_dst_image;
    delete m_pen;
  }

  // IToolLoop interface
  Context* getContext() { return m_context; }
  Tool* getTool() { return m_tool; }
  Pen* getPen() { return m_pen; }
  Document* getDocument() { return m_document; }
  Sprite* getSprite() { return m_sprite; }
  Layer* getLayer() { return m_layer; }
  Image* getSrcImage() { return m_src_image; }
  Image* getDstImage() { return m_dst_image; }
  bool useMask() { return m_useMask; }
  Mask* getMask() { return m_mask; }
  Point getMaskOrigin() { return m_maskOrigin; }
  int getMouseButton() { return m_button; }
  int getPrimaryColor() { return m_primary_color; }
  void setPrimaryColor(int color) { m_primary_color = color; }
  int getSecondaryColor() { return m_secondary_color; }
  void setSecondaryColor(int color) { m_secondary_color = color; }
  int getOpacity() { return m_opacity; }
  int getTolerance() { return m_tolerance; }
  TiledMode getTiledMode() { return m_tiled_mode; }
  bool getFilled() { return m_filled; }
  bool getPreviewFilled() { return m_previewFilled; }
  int getSprayWidth() { return m_sprayWidth; }
  int getSpraySpeed() { return m_spraySpeed; }
  Point getOffset() { return m_offset; }
  void setSpeed(const Point& speed) { m_speed = speed; }
  Point getSpeed() { return m_speed; }
  ToolInk* getInk() { return m_tool->getInk(m_button); }
  ToolController* getController() { return m_tool->getController(m_button); }
  ToolPointShape* getPointShape() { return m_tool->getPointShape(m_button); }
  ToolIntertwine* getIntertwine() { return m_tool->getIntertwine(m_button); }
  ToolTracePolicy getTracePolicy() { return m_tool->getTracePolicy(m_button); }

  void cancel() { m_canceled = true; }
  bool isCanceled() { return m_canceled; }

  Point screenToSprite(const Point& screenPoint)
  {
    Point spritePoint;
    m_editor->screenToEditor(screenPoint.x, screenPoint.y,
			     &spritePoint.x, &spritePoint.y);
    return spritePoint;
  }

  void updateArea(const Rect& dirty_area)
  {
    int x1 = dirty_area.x-m_offset.x;
    int y1 = dirty_area.y-m_offset.y;
    int x2 = dirty_area.x-m_offset.x+dirty_area.w-1;
    int y2 = dirty_area.y-m_offset.y+dirty_area.h-1;

    acquire_bitmap(ji_screen);
    editors_draw_sprite_tiled(m_sprite, x1, y1, x2, y2);
    release_bitmap(ji_screen);
  }

  void updateStatusBar(const char* text)
  {
    app_get_statusbar()->setStatusText(0, text);
  }

};

IToolLoop* Editor::createToolLoopImpl(Context* context, JMessage msg)
{
  Tool* current_tool = context->getSettings()->getCurrentTool();
  if (!current_tool)
    return NULL;

  Sprite* sprite = getSprite();
  Layer* layer = sprite->getCurrentLayer();

  if (!layer) {
    Alert::show(PACKAGE "<<The current sprite does not have any layer.||&Close");
    return NULL;
  }

  // If the active layer is not visible.
  if (!layer->is_readable()) {
    Alert::show(PACKAGE
		"<<The current layer is hidden,"
		"<<make it visible and try again"
		"||&Close");
    return NULL;
  }
  // If the active layer is read-only.
  else if (!layer->is_writable()) {
    Alert::show(PACKAGE
		"<<The current layer is locked,"
		"<<unlock it and try again"
		"||&Close");
    return NULL;
  }

  // Get fg/bg colors
  ColorBar* colorbar = app_get_colorbar();
  Color fg = colorbar->getFgColor();
  Color bg = colorbar->getBgColor();

  if (!fg.isValid() || !bg.isValid()) {
    Alert::show(PACKAGE
		"<<The current selected foreground and/or background color"
		"<<is out of range. Select valid colors in the color-bar."
		"||&Close");
    return NULL;
  }

  // Create the new tool loop
  ToolLoopImpl* tool_loop = new ToolLoopImpl(this,
					     context,
					     current_tool,
					     getDocument(),
					     sprite, layer,
					     msg->mouse.left ? 0: 1,
					     msg->mouse.left ? fg: bg,
					     msg->mouse.left ? bg: fg);

  return tool_loop;
}
