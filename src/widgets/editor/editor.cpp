/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include <stdio.h>
#include <allegro.h>

#include "jinete/jinete.h"

#include "ui_context.h"
#include "commands/commands.h"
#include "commands/params.h"
#include "app.h"
#include "core/cfg.h"
#include "core/color.h"
#include "modules/editors.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/skinneable_theme.h"
#include "modules/palettes.h"
#include "raster/raster.h"
#include "tools/tool.h"
#include "settings/settings.h"
#include "util/boundary.h"
#include "util/misc.h"
#include "util/recscr.h"
#include "util/render.h"
#include "widgets/colbar.h"
#include "widgets/editor.h"
#include "widgets/editor/pixels_movement.h"
#include "widgets/statebar.h"

#define has_shifts(msg,shift)			\
  (((msg)->any.shifts & (shift)) == (shift))

#define has_only_shifts(msg,shift)		\
  (((msg)->any.shifts & (KB_SHIFT_FLAG |	\
		         KB_ALT_FLAG |		\
		         KB_CTRL_FLAG)) == (shift))

static bool editor_view_msg_proc(JWidget widget, JMessage msg);

JWidget editor_view_new()
{
  JWidget widget = jview_new();
  SkinneableTheme* theme = static_cast<SkinneableTheme*>(widget->theme);
  int l = theme->get_part(PART_EDITOR_SELECTED_W)->w;
  int t = theme->get_part(PART_EDITOR_SELECTED_N)->h;
  int r = theme->get_part(PART_EDITOR_SELECTED_E)->w;
  int b = theme->get_part(PART_EDITOR_SELECTED_S)->h;

  jwidget_set_border(widget, l, t, r, b);
  jview_without_bars(widget);
  jwidget_add_hook(widget, JI_WIDGET, editor_view_msg_proc, NULL);

  return widget;
}

Editor::Editor()
  : Widget(editor_type())
{
  m_state = EDITOR_STATE_STANDBY;
  m_sprite = NULL;
  m_zoom = 0;

  m_cursor_thick = 0;
  m_cursor_screen_x = 0;
  m_cursor_screen_y = 0;
  m_cursor_editor_x = 0;
  m_cursor_editor_y = 0;
  m_old_cursor_thick = 0;

  m_cursor_candraw = false;
  m_insideSelection = false;
  m_alt_pressed = false;
  m_ctrl_pressed = false;
  m_space_pressed = false;

  m_offset_x = 0;
  m_offset_y = 0;
  m_mask_timer_id = jmanager_add_timer(this, 100);
  m_offset_count = 0;

  m_refresh_region = NULL;
  m_toolLoopManager = NULL;
  m_pixelsMovement = NULL;

  jwidget_focusrest(this, true);

  App::instance()->CurrentToolChange.connect(&Editor::onCurrentToolChange, this);
}

Editor::~Editor()
{
  jmanager_remove_timer(m_mask_timer_id);
  remove_editor(this);

  // Destroy tool-loop manager if it is created
  delete m_toolLoopManager;

  // Destroy all decorators
  deleteDecorators();
}

int editor_type()
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

void Editor::editor_set_sprite(Sprite* sprite)
{
  if (this->hasMouse())
    jmanager_free_mouse();	// TODO Why is this here? Review this code

  // Do we need to drop files?
  if (m_pixelsMovement)
    dropPixels();

  ASSERT(m_pixelsMovement == NULL && "You cannot change the current sprite while you are moving pixels");
  ASSERT(m_state == EDITOR_STATE_STANDBY && "You can change the current sprite only in stand-by state");

  // Change the sprite
  m_sprite = sprite;
  if (m_sprite) {
    // Get the preferred sprite's settings to edit it
    PreferredEditorSettings preferred = m_sprite->getPreferredEditorSettings();

    // Change the editor's configuration using the retrieved sprite's settings
    m_zoom = preferred.zoom;

    editor_update();
    editor_set_scroll(m_offset_x + preferred.scroll_x,
		      m_offset_y + preferred.scroll_y,
		      false);
  }
  // In this case sprite is NULL
  else {
    editor_update();
    editor_set_scroll(0, 0, false); // No scroll
  }

  // Redraw the entire editor (because we have a new sprite to draw)
  dirty();
}

// Sets the scroll position of the editor
void Editor::editor_set_scroll(int x, int y, int use_refresh_region)
{
  JWidget view = jwidget_get_view(this);
  int old_scroll_x, old_scroll_y;
  JRegion region = NULL;
  int thick = m_cursor_thick;

  if (thick)
    editor_clean_cursor();

  if (use_refresh_region) {
    region = jwidget_get_drawable_region(this, JI_GDR_CUTTOPWINDOWS);
    jview_get_scroll(view, &old_scroll_x, &old_scroll_y);
  }

  jview_set_scroll(view, x, y);

  if (m_sprite) {
    PreferredEditorSettings preferred;

    preferred.scroll_x = x - m_offset_x;
    preferred.scroll_y = y - m_offset_y;
    preferred.zoom = m_zoom;

    m_sprite->setPreferredEditorSettings(preferred);
  }

  if (use_refresh_region) {
    int new_scroll_x, new_scroll_y;

    jview_get_scroll(view, &new_scroll_x, &new_scroll_y);

    // Move screen with blits
    jwidget_scroll(this, region,
		   old_scroll_x - new_scroll_x,
		   old_scroll_y - new_scroll_y);
    
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

void Editor::editor_update()
{
  JWidget view = jwidget_get_view(this);
  jview_update(view);
}

/**
 * Draws the specified portion of sprite in the editor.
 *
 * @warning You should setup the clip of the @ref ji_screen before
 *          calling this routine.
 */
void Editor::editor_draw_sprite(int x1, int y1, int x2, int y2)
{
  JWidget view = jwidget_get_view(this);
  JRect vp = jview_get_viewport_position(view);
  int source_x, source_y, dest_x, dest_y, width, height;
  int scroll_x, scroll_y;

  // Get scroll

  jview_get_scroll(view, &scroll_x, &scroll_y);

  // Output information

  source_x = x1 << m_zoom;
  source_y = y1 << m_zoom;
  dest_x   = vp->x1 - scroll_x + m_offset_x + source_x;
  dest_y   = vp->y1 - scroll_y + m_offset_y + source_y;
  width    = (x2 - x1 + 1) << m_zoom;
  height   = (y2 - y1 + 1) << m_zoom;

  // Clip from viewport

  if (dest_x < vp->x1) {
    source_x += vp->x1 - dest_x;
    width -= vp->x1 - dest_x;
    dest_x = vp->x1;
  }

  if (dest_y < vp->y1) {
    source_y += vp->y1 - dest_y;
    height -= vp->y1 - dest_y;
    dest_y = vp->y1;
  }

  if (dest_x+width-1 > vp->x2-1)
    width = vp->x2-dest_x;

  if (dest_y+height-1 > vp->y2-1)
    height = vp->y2-dest_y;

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
    Image* rendered = RenderEngine::renderSprite(m_sprite,
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

  jrect_free(vp);

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

/**
 * Draws the sprite taking care of the whole clipping region.
 *
 * For each rectangle calls @ref editor_draw_sprite.
 */
void Editor::editor_draw_sprite_safe(int x1, int y1, int x2, int y2)
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
    editor_draw_sprite(x1, y1, x2, y2);
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
void Editor::editor_draw_mask()
{
  JWidget view = jwidget_get_view(this);
  JRect vp = jview_get_viewport_position(view);
  int scroll_x, scroll_y;
  int x1, y1, x2, y2;
  int c, x, y;

  jview_get_scroll(view, &scroll_x, &scroll_y);

  dotted_mode(m_offset_count);

  x = vp->x1 - scroll_x + m_offset_x;
  y = vp->y1 - scroll_y + m_offset_y;

  int nseg = m_sprite->getBoundariesSegmentsCount();
  const _BoundSeg* seg = m_sprite->getBoundariesSegments();

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

  jrect_free(vp);
}

void Editor::editor_draw_mask_safe()
{
  if ((m_sprite) && (m_sprite->getBoundariesSegments())) {
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
      editor_draw_mask();
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

void Editor::drawGrid(const Rect& gridBounds, color_t color)
{
  Rect grid(gridBounds);
  if (grid.w < 1 || grid.h < 1)
    return;

  int grid_color = get_color_for_allegro(bitmap_color_depth(ji_screen), color);
  JWidget view = jwidget_get_view(this);
  JRect vp = jview_get_viewport_position(view);
  int scroll_x, scroll_y;
  int x1, y1, x2, y2;
  int u1, v1, u2, v2;
  int c;

  jview_get_scroll(view, &scroll_x, &scroll_y);

  scroll_x = vp->x1 - scroll_x + m_offset_x;
  scroll_y = vp->y1 - scroll_y + m_offset_y;

  x1 = ji_screen->cl;
  y1 = ji_screen->ct;
  x2 = ji_screen->cr-1;
  y2 = ji_screen->cb-1;

  screen_to_editor(x1, y1, &u1, &v1);
  screen_to_editor(x2, y2, &u2, &v2);

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
  x1 = scroll_x+grid.x+u1*grid.w;
  x2 = scroll_x+grid.x+u2*grid.w;

  for (c=v1; c<=v2; c++)
    hline(ji_screen, x1, scroll_y+grid.y+c*grid.h, x2, grid_color);

  // Vertical lines
  y1 = scroll_y+grid.y+v1*grid.h;
  y2 = scroll_y+grid.y+v2*grid.h;

  for (c=u1; c<=u2; c++)
    vline(ji_screen, scroll_x+grid.x+c*grid.w, y1, y2, grid_color);

  jrect_free(vp);
}

void Editor::flashCurrentLayer()
{
  int x, y;
  const Image* src_image = m_sprite->getCurrentImage(&x, &y);
  if (src_image) {
    m_sprite->prepareExtraCel(0, 0, m_sprite->getWidth(), m_sprite->getHeight(), 255);
    Image* flash_image = m_sprite->getExtraCelImage();
    int u, v;

    image_clear(flash_image, flash_image->mask_color);
    for (v=0; v<flash_image->h; ++v) {
      for (u=0; u<flash_image->w; ++u) {
	if (u-x >= 0 && u-x < src_image->w &&
	    v-y >= 0 && v-y < src_image->h) {
	  ase_uint32 color = image_getpixel(src_image, u-x, v-y);
	  if (color != src_image->mask_color) {
	    color_t ccc = color_rgb(255, 255, 255);
	    image_putpixel(flash_image, u, v,
			   get_color_for_image(flash_image->imgtype, ccc));
	  }
	}
      }
    }

    editor_draw_sprite(0, 0, m_sprite->getWidth()-1, m_sprite->getHeight()-1);
    gui_flip_screen();

    vsync();
    rest(100);

    image_clear(flash_image, flash_image->mask_color);
    editor_draw_sprite(0, 0, m_sprite->getWidth()-1, m_sprite->getHeight()-1);
  }
}

void Editor::setMaskColorForPixelsMovement(color_t color)
{
  ASSERT(m_sprite != NULL);
  ASSERT(m_pixelsMovement != NULL);
  
  int imgtype = m_sprite->getImgType();
  m_pixelsMovement->setMaskColor(get_color_for_image(imgtype, color));
}

void Editor::deleteDecorators()
{
  for (std::vector<Decorator*>::iterator
	 it=m_decorators.begin(); it!=m_decorators.end(); ++it)
    delete *it;
  m_decorators.clear();
}

void Editor::addDecorator(Decorator* decorator)
{
  m_decorators.push_back(decorator);
}

void Editor::turnOnSelectionModifiers()
{
  deleteDecorators();

  int x1, y1, x2, y2;

  editor_to_screen(m_sprite->getMask()->x, m_sprite->getMask()->y, &x1, &y1);
  editor_to_screen(m_sprite->getMask()->x+m_sprite->getMask()->w-1,
		   m_sprite->getMask()->y+m_sprite->getMask()->h-1, &x2, &y2);

  addDecorator(new Decorator(Decorator::SELECTION_NW, Rect(x1-8,        y1-8, 8, 8)));
  addDecorator(new Decorator(Decorator::SELECTION_N,  Rect((x1+x2)/2-4, y1-8, 8, 8)));
  addDecorator(new Decorator(Decorator::SELECTION_NE, Rect(x2,          y1-8, 8, 8)));
  addDecorator(new Decorator(Decorator::SELECTION_E,  Rect(x2,          (y1+y2)/2-4, 8, 8)));
  addDecorator(new Decorator(Decorator::SELECTION_SE, Rect(x2,          y2, 8, 8)));
  addDecorator(new Decorator(Decorator::SELECTION_S,  Rect((x1+x2)/2-4, y2, 8, 8)));
  addDecorator(new Decorator(Decorator::SELECTION_SW, Rect(x1-8,        y2, 8, 8)));
  addDecorator(new Decorator(Decorator::SELECTION_W,  Rect(x1-8,        (y1+y2)/2-4, 8, 8)));
}

/**
   Control scroll when cursor goes out of the editor
   
   @param msg A mouse message received in Editor::onProcessMessage()
*/
void Editor::controlInfiniteScroll(JMessage msg)
{
  JWidget view = jwidget_get_view(this);
  JRect vp = jview_get_viewport_position(view);

  if (jmouse_control_infinite_scroll(vp)) {
    int scroll_x, scroll_y;
    int old_x = msg->mouse.x;
    int old_y = msg->mouse.y;

    msg->mouse.x = jmouse_x(0);
    msg->mouse.y = jmouse_y(0);

    // Smooth scroll movement
    if (get_config_bool("Options", "MoveSmooth", TRUE)) {
      jmouse_set_position(MID(vp->x1+1, old_x, vp->x2-2),
			  MID(vp->y1+1, old_y, vp->y2-2));
    }
    // This is better for high resolutions: scroll movement by big steps
    else {
      jmouse_set_position((old_x != msg->mouse.x) ?
			  (old_x + (vp->x1+vp->x2)/2)/2: msg->mouse.x,

			  (old_y != msg->mouse.y) ?
			  (old_y + (vp->y1+vp->y2)/2)/2: msg->mouse.y);
    }

    msg->mouse.x = jmouse_x(0);
    msg->mouse.y = jmouse_y(0);

    jview_get_scroll(view, &scroll_x, &scroll_y);
    editor_set_scroll(scroll_x+old_x-msg->mouse.x,
		      scroll_y+old_y-msg->mouse.y, true);
  }

  jrect_free(vp);
}

void Editor::dropPixels()
{
  if (m_pixelsMovement->isDragging())
    m_pixelsMovement->dropImageTemporarily();

  // Drop pixels if the user press a button outside the selection
  m_pixelsMovement->dropImage();
  delete m_pixelsMovement;
  m_pixelsMovement = NULL;

  m_sprite->destroyExtraCel();

  m_state = EDITOR_STATE_STANDBY;
  releaseMouse();

  app_get_statusbar()->hideMovePixelsOptions();
  editor_update_statusbar_for_standby();
}

void Editor::screen_to_editor(int xin, int yin, int *xout, int *yout)
{
  JWidget view = jwidget_get_view(this);
  JRect vp = jview_get_viewport_position(view);
  int scroll_x, scroll_y;

  jview_get_scroll(view, &scroll_x, &scroll_y);

  *xout = (xin - vp->x1 + scroll_x - m_offset_x) >> m_zoom;
  *yout = (yin - vp->y1 + scroll_y - m_offset_y) >> m_zoom;

  jrect_free(vp);
}

void Editor::editor_to_screen(int xin, int yin, int *xout, int *yout)
{
  JWidget view = jwidget_get_view(this);
  JRect vp = jview_get_viewport_position(view);
  int scroll_x, scroll_y;

  jview_get_scroll(view, &scroll_x, &scroll_y);

  *xout = (vp->x1 - scroll_x + m_offset_x + (xin << m_zoom));
  *yout = (vp->y1 - scroll_y + m_offset_y + (yin << m_zoom));

  jrect_free(vp);
}

void Editor::show_drawing_cursor()
{
  ASSERT(m_sprite != NULL);

  if (!m_cursor_thick && m_cursor_candraw) {
    jmouse_hide();
    editor_draw_cursor(jmouse_x(0), jmouse_y(0));
    jmouse_show();
  }
}

void Editor::hide_drawing_cursor()
{
  if (m_cursor_thick) {
    jmouse_hide();
    editor_clean_cursor();
    jmouse_show();
  }
}

void Editor::editor_update_statusbar_for_standby()
{
  UIContext* context = UIContext::instance();
  Tool* current_tool = context->getSettings()->getCurrentTool();
  int x, y;
  screen_to_editor(jmouse_x(0), jmouse_y(0), &x, &y);

  // For eye-dropper
  if (m_alt_pressed ||
      current_tool->getInk(0)->isEyedropper()) {
    int imgtype = m_sprite->getImgType();
    ase_uint32 pixel = m_sprite->getPixel(x, y);
    color_t color = color_from_image(imgtype, pixel);

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
    app_get_statusbar()->setStatusText
      (0, "Pos %d %d, Size %d %d, Frame %d",
       x, y,
       ((m_sprite->getMask()->bitmap)? m_sprite->getMask()->w: m_sprite->getWidth()),
       ((m_sprite->getMask()->bitmap)? m_sprite->getMask()->h: m_sprite->getHeight()),
       m_sprite->getCurrentFrame()+1);
  }
}

// Update status bar for when the user is dragging pixels
void Editor::editor_update_statusbar_for_pixel_movement()
{
  Rect bounds = m_pixelsMovement->getImageBounds();
  app_get_statusbar()->setStatusText
    (100, "Pos %d %d, Size %d %d",
     bounds.x, bounds.y, bounds.w, bounds.h);
}

void Editor::editor_refresh_region()
{
  if (this->update_region) {
    int thick = m_cursor_thick;

    if (thick)
      editor_clean_cursor();
    else
      jmouse_hide();

    // TODO check if this work!!!!
/*     jwidget_redraw_region */
/*       (jwindow_get_manager (jwidget_get_window (widget)), */
/*        m_refresh_region); */
/*     jwidget_redraw_region (widget, m_refresh_region); */
    jwidget_flush_redraw(this);
    jmanager_dispatch_messages(ji_get_default_manager());

/*     if (m_refresh_region) { */
/*       jregion_free (m_refresh_region); */
/*       m_refresh_region = NULL; */
/*     } */

    if (thick)
      editor_draw_cursor(m_cursor_screen_x, m_cursor_screen_y);
    else
      jmouse_show();
  }
}

//////////////////////////////////////////////////////////////////////
// Message handler for the a view widget that contains an editor

static bool editor_view_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_SETPOS:
      // This avoid the displacement of the widgets in the viewport

      jrect_copy(widget->rc, &msg->setpos.rect);
      jview_update(widget);
      return true;

    case JM_DRAW:
      {
	JWidget viewport = jview_get_viewport(widget);
	JWidget child = reinterpret_cast<JWidget>(jlist_first_data(viewport->children));
	JRect pos = jwidget_get_rect(widget);
	SkinneableTheme* theme = static_cast<SkinneableTheme*>(widget->theme);

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
      SkinneableTheme* theme = static_cast<SkinneableTheme*>(this->theme);

      if (m_old_cursor_thick == 0) {
      	m_old_cursor_thick = m_cursor_thick;
      }

      if (m_cursor_thick)
      	editor_clean_cursor();

      // Editor without sprite
      if (!m_sprite) {
	JWidget view = jwidget_get_view(this);
	JRect vp = jview_get_viewport_position(view);

	jdraw_rectfill(vp, theme->get_editor_face_color());
	draw_emptyset_symbol(ji_screen,
			     Rect(vp->x1, vp->y1, jrect_w(vp), jrect_h(vp)),
			     makecol(64, 64, 64));

	jrect_free(vp);
      }
      // Editor with sprite
      else {
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
	editor_draw_sprite(0, 0, m_sprite->getWidth()-1, m_sprite->getHeight()-1);

	// Draw the sprite boundary
	rect(ji_screen, x1-1, y1-1, x2+1, y2+1, theme->get_editor_sprite_border());
	hline(ji_screen, x1-1, y2+2, x2+1, theme->get_editor_sprite_bottom_edge());

	// Draw the mask boundaries
	if (m_sprite->getBoundariesSegments()) {
	  editor_draw_mask();
	  jmanager_start_timer(m_mask_timer_id);
	}
	else {
	  jmanager_stop_timer(m_mask_timer_id);
	}

	// Draw decorators
	for (std::vector<Decorator*>::iterator
	       it=m_decorators.begin(); it!=m_decorators.end(); ++it) {
	  (*it)->drawDecorator(this, ji_screen);
	}

	if (msg->draw.count == 0
	    && m_old_cursor_thick != 0) {
	  editor_draw_cursor(jmouse_x(0), jmouse_y(0));
	}
      }

      if (msg->draw.count == 0)
      	m_old_cursor_thick = 0;

      return true;
    }

    case JM_TIMER:
      if (msg->timer.timer_id == m_mask_timer_id) {
	if (m_sprite) {
	  editor_draw_mask_safe();

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

      if (msg->any.shifts & KB_ALT_FLAG) m_alt_pressed = true;
      if (msg->any.shifts & KB_CTRL_FLAG) m_ctrl_pressed = true;
      if (key[KEY_SPACE]) m_space_pressed = true;
      break;

    case JM_MOUSELEAVE:
      hide_drawing_cursor();

      if (m_alt_pressed) m_alt_pressed = false;
      if (m_ctrl_pressed) m_ctrl_pressed = false;
      if (m_space_pressed) m_space_pressed = false;

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
	  update_screen_for_sprite(m_sprite);

	  clear_keybuf();

	  editor_setcursor(msg->mouse.x, msg->mouse.y);

	  releaseMouse();
	  return true;
	}
      }

      if (!hasCapture()) {
	UIContext* context = UIContext::instance();
	Tool* current_tool = context->getSettings()->getCurrentTool();

	set_current_editor(this);
	context->set_current_sprite(m_sprite);

	// Start scroll loop
	if (msg->mouse.middle ||
	    m_space_pressed ||
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
	    screen_to_editor(msg->mouse.x, msg->mouse.y, &x, &y);
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
	if ((m_ctrl_pressed &&
	     !current_tool->getInk(msg->mouse.right ? 1: 0)->isSelection()) ||
	    current_tool->getInk(msg->mouse.right ? 1: 0)->isCelMovement()) {
	  if ((m_sprite->getCurrentLayer()) &&
	      (m_sprite->getCurrentLayer()->type == GFXOBJ_LAYER_IMAGE)) {
	    // TODO you can move the `Background' with tiled mode
	    if (m_sprite->getCurrentLayer()->is_background()) {
	      jalert(_(PACKAGE
		       "<<You can't move the `Background' layer."
		       "||&Close"));
	    }
	    else if (!m_sprite->getCurrentLayer()->is_moveable()) {
	      jalert(_(PACKAGE
		       "<<The layer movement is locked."
		       "||&Close"));
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
	      jalert(_(PACKAGE
		       "<<The layer is locked."
		       "||&Close"));
	      return true;
	    }

	    // Copy the mask to the extra cel image
	    Image* tmpImage = NewImageFromMask(m_sprite);
	    x = m_sprite->getMask()->x;
	    y = m_sprite->getMask()->y;
	    m_pixelsMovement = new PixelsMovement(m_sprite, tmpImage, x, y, opacity);
	    delete tmpImage;

	    // If the CTRL key is pressed start dragging a copy of the selection
	    if (m_ctrl_pressed)
	      m_pixelsMovement->copyMask();
	    else
	      m_pixelsMovement->cutMask();

	    screen_to_editor(msg->mouse.x, msg->mouse.y, &x, &y);
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
	else if (m_alt_pressed ||
		 current_tool->getInk(msg->mouse.right ? 1: 0)->isEyedropper()) {
	  Command* eyedropper_cmd = 
	    CommandsModule::instance()->get_command_by_name(CommandId::eyedropper);

	  Params params;
	  params.set("target", msg->mouse.right ? "background": "foreground");

	  UIContext::instance()->execute_command(eyedropper_cmd, &params);
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
	JWidget view = jwidget_get_view(this);
	JRect vp = jview_get_viewport_position(view);
	int scroll_x, scroll_y;

	jview_get_scroll(view, &scroll_x, &scroll_y);
	editor_set_scroll(scroll_x+jmouse_x(1)-jmouse_x(0),
			  scroll_y+jmouse_y(1)-jmouse_y(0), true);

	jmouse_control_infinite_scroll(vp);
	jrect_free(vp);

	{
	  int x, y;
	  screen_to_editor(jmouse_x(0), jmouse_y(0), &x, &y);
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
	  screen_to_editor(msg->mouse.x, msg->mouse.y, &x, &y);

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
	update_screen_for_sprite(m_sprite);

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
	switch (msg->key.scancode) {
	  
	  // Eye-dropper is activated with ALT key
	  case KEY_ALT:
	    m_alt_pressed = true;
	    editor_setcursor(jmouse_x(0), jmouse_y(0));
	    return true;

	  case KEY_LCONTROL:
	  case KEY_RCONTROL:
	    // If the user press the CTRL key when he is dragging pixels (but not pressing the mouse buttons)...
	    if (!m_ctrl_pressed && !jmouse_b(0) && m_pixelsMovement) {
	      // Drop pixels (sure the user will press the mouse button to start dragging a copy)
	      dropPixels();
	    }
	    m_ctrl_pressed = true;
	    editor_setcursor(jmouse_x(0), jmouse_y(0));
	    return true;

	  case KEY_SPACE:
	    m_space_pressed = true;
	    editor_setcursor(jmouse_x(0), jmouse_y(0));
	    return true;
	}
      }

      // When we are drawing, we "eat" all pressed keys
      if (m_state == EDITOR_STATE_DRAWING)
	return true;

      break;

    case JM_KEYRELEASED:
      switch (msg->key.scancode) {

	// Eye-dropper is deactivated with ALT key
	case KEY_ALT:
	  if (m_alt_pressed) {
	    m_alt_pressed = false;
	    editor_setcursor(jmouse_x(0), jmouse_y(0));
	    return true;
	  }
	  break;

	  case KEY_LCONTROL:
	  case KEY_RCONTROL:
	    if (m_ctrl_pressed) {
	      m_ctrl_pressed = false;
	      editor_setcursor(jmouse_x(0), jmouse_y(0));
	      return true;
	    }
	    break;

	case KEY_SPACE:
	  if (m_space_pressed) {
	    // We have to clear all the KEY_SPACE in buffer
	    clear_keybuf();

	    m_space_pressed = false;
	    editor_setcursor(jmouse_x(0), jmouse_y(0));
	    return true;
	  }
	  break;
      }
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
		if (color_type(app_get_colorbar()->getFgColor()) == COLOR_TYPE_INDEX) {
		  newIndex = color_get_index(app_get_colorbar()->getFgColor()) + dz;
		  newIndex = MID(0, newIndex, 255);
		}
		app_get_colorbar()->setFgColor(color_index(newIndex));
	      }
	      break;

	    case WHEEL_BG:
	      if (m_state == EDITOR_STATE_STANDBY) {
		int newIndex = 0;
		if (color_type(app_get_colorbar()->getBgColor()) == COLOR_TYPE_INDEX) {
		  newIndex = color_get_index(app_get_colorbar()->getBgColor()) + dz;
		  newIndex = MID(0, newIndex, 255);
		}
		app_get_colorbar()->setBgColor(color_index(newIndex));
	      }
	      break;

	    case WHEEL_FRAME:
	      if (m_state == EDITOR_STATE_STANDBY) {
		Command* command = CommandsModule::instance()->get_command_by_name
		  ((dz < 0) ? CommandId::goto_next_frame: CommandId::goto_previous_frame);
		if (command)
		  UIContext::instance()->execute_command(command, NULL);
	      }
	      break;

	    case WHEEL_ZOOM: {
	      int zoom = MID(MIN_ZOOM, m_zoom-dz, MAX_ZOOM);
	      if (m_zoom != zoom)
		editor_set_zoom_and_center_in_mouse(zoom, msg->mouse.x, msg->mouse.y);
	      break;
	    }

	    case WHEEL_HSCROLL:
	    case WHEEL_VSCROLL: {
	      JWidget view = jwidget_get_view(this);
	      JRect vp = jview_get_viewport_position(view);
	      int scroll_x, scroll_y;
	      int dx = 0;
	      int dy = 0;
	      int thick = m_cursor_thick;

	      if (wheelAction == WHEEL_HSCROLL) {
		dx = dz * jrect_w(vp);
	      }
	      else {
		dy = dz * jrect_h(vp);
	      }

	      if (scrollBigSteps) {
		dx /= 2;
		dy /= 2;
	      }
	      else {
		dx /= 10;
		dy /= 10;
	      }
		
	      jview_get_scroll(view, &scroll_x, &scroll_y);

	      jmouse_hide();
	      if (thick)
		editor_clean_cursor();
	      editor_set_scroll(scroll_x+dx, scroll_y+dy, true);
	      if (thick)
		editor_draw_cursor(jmouse_x(0), jmouse_y(0));
	      jmouse_show();

	      jrect_free(vp);
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
  UIContext* context = UIContext::instance();
  Tool* current_tool = context->getSettings()->getCurrentTool();

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
    JWidget view = jwidget_get_view(this);
    JRect vp = jview_get_viewport_position(view);

    m_offset_x = jrect_w(vp)/2 - 1;
    m_offset_y = jrect_h(vp)/2 - 1;

    *w = (m_sprite->getWidth() << m_zoom) + m_offset_x*2;
    *h = (m_sprite->getHeight() << m_zoom) + m_offset_y*2;

    jrect_free(vp);
  }
  else {
    *w = 4;
    *h = 4;
  }
}

void Editor::editor_setcursor(int x, int y)
{
  UIContext* context = UIContext::instance();
  Tool* current_tool = context->getSettings()->getCurrentTool();

  switch (m_state) {

    case EDITOR_STATE_SCROLLING:
      hide_drawing_cursor();
      jmouse_set_cursor(JI_CURSOR_SCROLL);
      break;

    case EDITOR_STATE_DRAWING:
      if (current_tool->getInk(0)->isEyedropper()) {
	hide_drawing_cursor();
	jmouse_set_cursor(JI_CURSOR_EYEDROPPER);
	return;
      }
      else {
	jmouse_set_cursor(JI_CURSOR_NULL);
	show_drawing_cursor();
      }
      break;

    case EDITOR_STATE_STANDBY:
      if (m_sprite) {
	UIContext* context = UIContext::instance();
	Tool* current_tool = context->getSettings()->getCurrentTool();

	editor_update_candraw(); // TODO remove this

	// Pixels movement
	if (m_pixelsMovement) {
	  int x, y;
	  screen_to_editor(jmouse_x(0), jmouse_y(0), &x, &y);
	    
	  // Move selection
	  if (m_pixelsMovement->isDragging() ||
	      m_sprite->getMask()->contains_point(x, y)) {
	    hide_drawing_cursor();
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
	    show_drawing_cursor();
	  }
	  // Forbidden
	  else {
	    hide_drawing_cursor();
	    jmouse_set_cursor(JI_CURSOR_FORBIDDEN);
	  }
	}
	// Eyedropper
	else if (m_alt_pressed) {
	  hide_drawing_cursor();
	  jmouse_set_cursor(JI_CURSOR_EYEDROPPER);
	}
	// Move layer
	else if (m_ctrl_pressed &&
		 (!current_tool ||
		  !current_tool->getInk(0)->isSelection())) {
	  hide_drawing_cursor();
	  jmouse_set_cursor(JI_CURSOR_MOVE);
	}
	// Scroll
	else if (m_space_pressed) {
	  hide_drawing_cursor();
	  jmouse_set_cursor(JI_CURSOR_SCROLL);
	}
	else {
	  if (current_tool) {
	    // If the current tool change selection (e.g. rectangular marquee, etc.)
	    if (current_tool->getInk(0)->isSelection()) {
	      int x, y;
	      screen_to_editor(jmouse_x(0), jmouse_y(0), &x, &y);
	    
	      // Move pixels
	      if (m_sprite->getMask()->contains_point(x, y)) {
		hide_drawing_cursor();
		if (m_ctrl_pressed)
		  jmouse_set_cursor(JI_CURSOR_NORMAL_ADD);
		else
		  jmouse_set_cursor(JI_CURSOR_MOVE);

		if (!m_insideSelection) {
		  m_insideSelection = true;
		  //turnOnSelectionModifiers();
		}
		return;
	      }
	    }
	    else if (current_tool->getInk(0)->isEyedropper()) {
	      hide_drawing_cursor();
	      jmouse_set_cursor(JI_CURSOR_EYEDROPPER);
	      return;
	    }
	    else if (current_tool->getInk(0)->isScrollMovement()) {
	      hide_drawing_cursor();
	      jmouse_set_cursor(JI_CURSOR_SCROLL);
	      return;
	    }
	  }

	  if (m_insideSelection) {
	    m_insideSelection = false;
	    //deleteDecorators();
	  }

	  // Draw
	  if (m_cursor_candraw) {
	    jmouse_set_cursor(JI_CURSOR_NULL);
	    show_drawing_cursor();
	  }
	  // Forbidden
	  else {
	    hide_drawing_cursor();
	    jmouse_set_cursor(JI_CURSOR_FORBIDDEN);
	  }
	}
      }
      else {
	hide_drawing_cursor();
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

void Editor::editor_set_zoom_and_center_in_mouse(int zoom, int mouse_x, int mouse_y)
{
  JWidget view = jwidget_get_view(this);
  JRect vp = jview_get_viewport_position(view);
  int x, y;

  hide_drawing_cursor();
  screen_to_editor(mouse_x, mouse_y, &x, &y);

  x = m_offset_x - jrect_w(vp)/2 + ((1<<zoom)>>1) + (x << zoom);
  y = m_offset_y - jrect_h(vp)/2 + ((1<<zoom)>>1) + (y << zoom);

  if ((m_zoom != zoom) ||
      (m_cursor_editor_x != (vp->x1+vp->x2)/2) ||
      (m_cursor_editor_y != (vp->y1+vp->y2)/2)) {
    int use_refresh_region = (m_zoom == zoom) ? true: false;

    m_zoom = zoom;

    editor_update();
    editor_set_scroll(x, y, use_refresh_region);

    jmouse_set_position((vp->x1+vp->x2)/2, (vp->y1+vp->y2)/2);
  }
  show_drawing_cursor();
  jrect_free(vp);
}

//////////////////////////////////////////////////////////////////////
// Decorator implementation

Editor::Decorator::Decorator(Type type, const Rect& bounds)
{
  m_type = type;
  m_bounds = bounds;
}

Editor::Decorator::~Decorator()
{
}

void Editor::Decorator::drawDecorator(Editor* editor, BITMAP* bmp)
{
  rect(bmp,
       m_bounds.x, m_bounds.y,
       m_bounds.x+m_bounds.w-1,
       m_bounds.y+m_bounds.h-1, makecol(255, 255, 255));

  rectfill(bmp,
	   m_bounds.x+1, m_bounds.y+1,
	   m_bounds.x+m_bounds.w-2, m_bounds.y+m_bounds.h-2,
	   makecol(0, 0, 0));
}

bool Editor::Decorator::isInsideDecorator(int x, int y)
{
  return (m_bounds.contains(Point(x, y)));
}

//////////////////////////////////////////////////////////////////////
// IToolLoop implementation

class ToolLoopImpl : public IToolLoop
{
  Editor* m_editor;
  Context* m_context;
  Tool* m_tool;
  Pen* m_pen;
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
  ToolLoopImpl(Editor* editor, Context* context, Tool* tool, Sprite* sprite, Layer* layer,
	       int button, color_t primary_color, color_t secondary_color)
  {
    m_editor = editor;
    m_context = context;
    m_tool = tool;
    m_sprite = sprite;
    m_layer = layer;
    m_cel = NULL;
    m_cel_image = NULL;
    m_cel_created = false;
    m_canceled = false;
    m_button = button;
    m_primary_color = get_color_for_layer(layer, primary_color);
    m_secondary_color = get_color_for_layer(layer, secondary_color);

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
      m_cel = static_cast<LayerImage*>(sprite->getCurrentLayer())->get_cel(sprite->getCurrentFrame());
      if (m_cel)
	m_cel_image = sprite->getStock()->image[m_cel->image];
    }

    if (m_cel == NULL) {
      // create the image
      m_cel_image = image_new(sprite->getImgType(), sprite->getWidth(), sprite->getHeight());
      image_clear(m_cel_image, 0);

      // create the cel
      m_cel = cel_new(sprite->getCurrentFrame(), 0);
      static_cast<LayerImage*>(sprite->getCurrentLayer())->add_cel(m_cel);

      m_cel_created = true;
    }

    m_old_cel_x = m_cel->x;
    m_old_cel_y = m_cel->y;

    // region to draw
    int x1, y1, x2, y2;

    // non-tiled
    if (m_tiled_mode == TILED_NONE) {
      x1 = MIN(m_cel->x, 0);
      y1 = MIN(m_cel->y, 0);
      x2 = MAX(m_cel->x+m_cel_image->w, m_sprite->getWidth());
      y2 = MAX(m_cel->y+m_cel_image->h, m_sprite->getHeight());
    }
    else { 			// tiled
      x1 = 0;
      y1 = 0;
      x2 = m_sprite->getWidth();
      y2 = m_sprite->getHeight();
    }

    // create two copies of the image region which we'll modify with the tool
    m_src_image = image_crop(m_cel_image,
			     x1-m_cel->x,
			     y1-m_cel->y, x2-x1, y2-y1, 0);
    m_dst_image = image_new_copy(m_src_image);

    m_mask = m_sprite->getMask();
    m_maskOrigin = (!m_mask->is_empty() ? Point(m_mask->x-x1, m_mask->y-y1):
					  Point(0, 0));

    m_opacity = settings->getToolSettings(m_tool)->getOpacity();
    m_tolerance = settings->getToolSettings(m_tool)->getTolerance();
    m_speed.x = 0;
    m_speed.y = 0;

    // we have to modify the cel position because it's used in the
    // `render_sprite' routine to draw the `dst_image'
    m_cel->x = x1;
    m_cel->y = y1;
    m_offset.x = -x1;
    m_offset.y = -y1;

    // Set undo label for any kind of undo used in the whole loop
    if (undo_is_enabled(m_sprite->getUndo()))
      undo_set_label(m_sprite->getUndo(), m_tool->getText().c_str());
  }

  ~ToolLoopImpl()
  {
    if (!m_canceled) {
      // Paint ink
      if (getInk()->isPaint()) {
	/* if the size of each image is the same, we can create an undo
	   with only the differences between both images */
	if (m_cel->x == m_old_cel_x &&
	    m_cel->y == m_old_cel_y &&
	    m_cel_image->w == m_dst_image->w &&
	    m_cel_image->h == m_dst_image->h) {
	  /* was the 'cel_image' created in the start of the tool-loop? */
	  if (m_cel_created) {
	    /* then we can keep the 'cel_image'... */
	  
	    /* we copy the 'destination' image to the 'cel_image' */
	    image_copy(m_cel_image, m_dst_image, 0, 0);

	    /* add the 'cel_image' in the images' stock of the sprite */
	    m_cel->image = stock_add_image(m_sprite->getStock(), m_cel_image);

	    /* is the undo enabled? */
	    if (undo_is_enabled(m_sprite->getUndo())) {
	      /* we can temporary remove the cel */
	      static_cast<LayerImage*>(m_sprite->getCurrentLayer())->remove_cel(m_cel);

	      /* we create the undo information (for the new cel_image
		 in the stock and the new cel in the layer)... */
	      undo_open(m_sprite->getUndo());
	      undo_add_image(m_sprite->getUndo(), m_sprite->getStock(), m_cel->image);
	      undo_add_cel(m_sprite->getUndo(), m_sprite->getCurrentLayer(), m_cel);
	      undo_close(m_sprite->getUndo());

	      /* and finally we add the cel again in the layer */
	      static_cast<LayerImage*>(m_sprite->getCurrentLayer())->add_cel(m_cel);
	    }
	  }
	  else {
	    /* undo the dirty region */
	    if (undo_is_enabled(m_sprite->getUndo())) {
	      Dirty* dirty = dirty_new_from_differences(m_cel_image,
							m_dst_image);
	      // TODO error handling

	      dirty_save_image_data(dirty);
	      if (dirty != NULL)
		undo_dirty(m_sprite->getUndo(), dirty);

	      dirty_free(dirty);
	    }

	    /* copy the 'dst_image' to the cel_image */
	    image_copy(m_cel_image, m_dst_image, 0, 0);
	  }
	}
	/* if the size of both images are different, we have to replace
	   the entire image */
	else {
	  if (undo_is_enabled(m_sprite->getUndo())) {
	    undo_open(m_sprite->getUndo());

	    if (m_cel->x != m_old_cel_x) {
	      int x = m_cel->x;
	      m_cel->x = m_old_cel_x;
	      undo_int(m_sprite->getUndo(), (GfxObj*)m_cel, &m_cel->x);
	      m_cel->x = x;
	    }
	    if (m_cel->y != m_old_cel_y) {
	      int y = m_cel->y;
	      m_cel->y = m_old_cel_y;
	      undo_int(m_sprite->getUndo(), (GfxObj*)m_cel, &m_cel->y);
	      m_cel->y = y;
	    }

	    undo_replace_image(m_sprite->getUndo(), m_sprite->getStock(), m_cel->image);
	    undo_close(m_sprite->getUndo());
	  }

	  /* replace the image in the stock */
	  stock_replace_image(m_sprite->getStock(), m_cel->image, m_dst_image);

	  /* destroy the old cel image */
	  image_free(m_cel_image);

	  /* now the `dst_image' is used, so we haven't to destroy it */
	  m_dst_image = NULL;
	}
      }

      // Selection ink
      if (getInk()->isSelection())
	m_sprite->generateMaskBoundaries();
    }

    // If the trace was not canceled or it is not a 'paint' ink...
    if (m_canceled || !getInk()->isPaint()) {
      // Here we destroy the temporary 'cel' created and restore all as it was before

      m_cel->x = m_old_cel_x;
      m_cel->y = m_old_cel_y;

      if (m_cel_created) {
	static_cast<LayerImage*>(m_layer)->remove_cel(m_cel);
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
  Sprite* getSprite() { return m_sprite; }
  Layer* getLayer() { return m_layer; }
  Image* getSrcImage() { return m_src_image; }
  Image* getDstImage() { return m_dst_image; }
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
    m_editor->screen_to_editor(screenPoint.x, screenPoint.y,
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
    jalert(_(PACKAGE
	     "<<The current sprite does not have any layer."
	     "||&Close"));
    return NULL;
  }

  // if the active layer is not visible
  if (!layer->is_readable()) {
    jalert(_(PACKAGE
	     "<<The current layer is hidden,"
	     "<<make it visible and try again"
	     "||&Close"));
    return NULL;
  }
  // if the active layer is read-only
  else if (!layer->is_writable()) {
    jalert(_(PACKAGE
	     "<<The current layer is locked,"
	     "<<unlock it and try again"
	     "||&Close"));
    return NULL;
  }

  // Get fg/bg colors
  ColorBar* colorbar = app_get_colorbar();
  color_t fg = colorbar->getFgColor();
  color_t bg = colorbar->getBgColor();

  if (!color_is_valid(fg) || !color_is_valid(bg)) {
    jalert(PACKAGE
	   "<<The current selected foreground and/or background color"
	   "<<is out of range. Select valid colors in the color-bar."
	   "||&Close");
    return NULL;
  }

  // Create the new tool loop
  ToolLoopImpl* tool_loop = new ToolLoopImpl(this,
					     context,
					     current_tool, sprite, layer,
					     msg->mouse.left ? 0: 1,
					     msg->mouse.left ? fg: bg,
					     msg->mouse.left ? bg: fg);

  return tool_loop;
}
