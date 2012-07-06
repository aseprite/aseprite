/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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
#include "base/bind.h"
#include "commands/commands.h"
#include "commands/params.h"
#include "document_wrappers.h"
#include "ini_file.h"
#include "modules/editors.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "raster/raster.h"
#include "settings/settings.h"
#include "skin/skin_theme.h"
#include "tools/ink.h"
#include "tools/tool.h"
#include "tools/tool_box.h"
#include "ui/gui.h"
#include "ui_context.h"
#include "util/boundary.h"
#include "util/misc.h"
#include "util/render.h"
#include "widgets/color_bar.h"
#include "widgets/editor/editor_customization_delegate.h"
#include "widgets/editor/editor_decorator.h"
#include "widgets/editor/moving_pixels_state.h"
#include "widgets/editor/pixels_movement.h"
#include "widgets/editor/standby_state.h"
#include "widgets/status_bar.h"
#include "widgets/toolbar.h"

#include <allegro.h>
#include <stdio.h>

using namespace gfx;
using namespace ui;

class EditorPreRenderImpl : public EditorPreRender
{
public:
  EditorPreRenderImpl(Editor* editor, Image* image, const Point& offset, int zoom)
    : m_editor(editor)
    , m_image(image)
    , m_offset(offset)
    , m_zoom(zoom)
  {
  }

  Editor* getEditor() OVERRIDE
  {
    return m_editor;
  }

  Image* getImage() OVERRIDE
  {
    return m_image;
  }

  void fillRect(const gfx::Rect& rect, uint32_t rgbaColor, int opacity) OVERRIDE
  {
    image_rectblend(m_image,
                    m_offset.x + (rect.x << m_zoom),
                    m_offset.y + (rect.y << m_zoom),
                    m_offset.x + ((rect.x+rect.w) << m_zoom) - 1,
                    m_offset.y + ((rect.y+rect.h) << m_zoom) - 1, rgbaColor, opacity);
  }

private:
  Editor* m_editor;
  Image* m_image;
  Point m_offset;
  int m_zoom;
};

class EditorPostRenderImpl : public EditorPostRender
{
public:
  EditorPostRenderImpl(Editor* editor)
    : m_editor(editor)
  {
  }

  Editor* getEditor()
  {
    return m_editor;
  }

  void drawLine(int x1, int y1, int x2, int y2, int screenColor)
  {
    int u1, v1, u2, v2;
    m_editor->editorToScreen(x1, y1, &u1, &v1);
    m_editor->editorToScreen(x2, y2, &u2, &v2);
    line(ji_screen, u1, v1, u2, v2, screenColor);
  }

private:
  Editor* m_editor;
};

Editor::Editor()
  : Widget(editor_type())
  , m_state(new StandbyState())
  , m_decorator(NULL)
  , m_mask_timer(100, this)
  , m_customizationDelegate(NULL)
{
  // Add the first state into the history.
  m_statesHistory.push(m_state);

  m_document = NULL;
  m_sprite = NULL;
  m_zoom = 0;

  m_cursor_thick = 0;
  m_cursor_screen_x = 0;
  m_cursor_screen_y = 0;
  m_cursor_editor_x = 0;
  m_cursor_editor_y = 0;

  m_quicktool = NULL;

  m_offset_x = 0;
  m_offset_y = 0;
  m_offset_count = 0;

  this->setFocusStop(true);

  m_currentToolChangeSlot =
    App::instance()->CurrentToolChange.connect(&Editor::onCurrentToolChange, this);

  m_fgColorChangeSlot =
    app_get_colorbar()->FgColorChange.connect(Bind<void>(&Editor::onFgColorChange, this));
}

Editor::~Editor()
{
  setCustomizationDelegate(NULL);

  m_mask_timer.stop();
  remove_editor(this);

  // Remove this editor as listener of CurrentToolChange signal.
  App::instance()->CurrentToolChange.disconnect(m_currentToolChangeSlot);
  delete m_currentToolChangeSlot;

  // Remove this editor as listener of FgColorChange
  app_get_colorbar()->FgColorChange.disconnect(m_fgColorChangeSlot);
  delete m_fgColorChangeSlot;
}

int editor_type()
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

void Editor::setStateInternal(const EditorStatePtr& newState)
{
  hideDrawingCursor();

  // Fire before change state event, set the state, and fire after
  // change state event.
  EditorState::BeforeChangeAction beforeChangeAction =
    m_state->onBeforeChangeState(this, newState);

  // Push a new state
  if (newState) {
    if (beforeChangeAction == EditorState::DiscardState)
      m_statesHistory.pop();

    m_statesHistory.push(newState);
    m_state = newState;
  }
  // Go to previous state
  else {
    m_state->onBeforePopState(this);

    m_statesHistory.pop();
    m_state = m_statesHistory.top();
  }

  ASSERT(m_state != NULL);

  // Change to the new state.
  m_state->onAfterChangeState(this);

  // Redraw all the editors with the same document of this editor
  update_screen_for_document(m_document);

  // Clear keyboard buffer just in case (to avoid sending keys to the
  // new state).
  clear_keybuf();

  // Notify listeners
  m_listeners.notifyStateChanged(this);

  // Setup the new mouse cursor
  editor_setcursor();

  updateStatusBar();
}

void Editor::setState(const EditorStatePtr& newState)
{
  setStateInternal(newState);
}

void Editor::backToPreviousState()
{
  setStateInternal(EditorStatePtr(NULL));
}

void Editor::setDocument(Document* document)
{
  //if (this->hasMouse())
  //jmanager_free_mouse();      // TODO Why is this here? Review this code

  // Reset all states (back to standby).
  EditorStatePtr firstState(new StandbyState);
  m_statesHistory.clear();
  m_statesHistory.push(firstState);
  setState(firstState);

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

  // Notify listeners
  m_listeners.notifyDocumentChanged(this);
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

  if (m_document && changePreferredSettings()) {
    PreferredEditorSettings preferred;

    preferred.virgin = false;
    preferred.scroll_x = newScroll.x - m_offset_x;
    preferred.scroll_y = newScroll.y - m_offset_y;
    preferred.zoom = m_zoom;

    m_document->setPreferredEditorSettings(preferred);
  }

  if (use_refresh_region) {
    // Move screen with blits
    this->scrollRegion(region,
                       oldScroll.x - newScroll.x,
                       oldScroll.y - newScroll.y);

    jregion_free(region);
  }

  if (thick)
    editor_draw_cursor(m_cursor_screen_x, m_cursor_screen_y);

  // Notify listeners
  m_listeners.notifyScrollChanged(this);
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
      // Pre-render decorator.
      if (m_decorator) {
        EditorPreRenderImpl preRender(this, rendered,
                                      Point(-source_x, -source_y), m_zoom);
        m_decorator->preRenderDecorator(&preRender);
      }

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

  // Post-render decorator.
  if (m_decorator) {
    EditorPostRenderImpl postRender(this);
    m_decorator->postRenderDecorator(&postRender);
  }
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

#if 1                           // Bounds inside mask
    if (!seg->open)
#else                           // Bounds outside mask
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
  if (isVisible() &&
      m_document &&
      m_document->getBoundariesSegments()) {
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
  // Copy the grid bounds
  Rect grid(gridBounds);
  if (grid.w < 1 || grid.h < 1)
    return;

  // Move the grid bounds to a non-negative position.
  if (grid.x < 0) grid.x += (ABS(grid.x)/grid.w+1) * grid.w;
  if (grid.y < 0) grid.y += (ABS(grid.y)/grid.h+1) * grid.h;

  // Change the grid position to the first grid's tile
  grid.setOrigin(Point((grid.x % grid.w) - grid.w,
                       (grid.y % grid.h) - grid.h));
  if (grid.x < 0) grid.x += grid.w;
  if (grid.y < 0) grid.y += grid.h;

  // Convert the "grid" rectangle to screen coordinates
  editorToScreen(grid, &grid);

  // Get the grid's color
  int grid_color = color_utils::color_for_allegro(color, bitmap_color_depth(ji_screen));

  // Get the position of the sprite in the screen.
  Rect spriteRect;
  editorToScreen(Rect(0, 0, m_sprite->getWidth(), m_sprite->getHeight()), &spriteRect);

  // Draw horizontal lines
  int x1 = spriteRect.x;
  int y1 = grid.y;
  int x2 = spriteRect.x+spriteRect.w;
  int y2 = spriteRect.y+spriteRect.h;

  for (int c=y1; c<=y2; c+=grid.h)
    hline(ji_screen, x1, c, x2, grid_color);

  // Draw vertical lines
  x1 = grid.x;
  y1 = spriteRect.y;

  for (int c=x1; c<=x2; c+=grid.w)
    vline(ji_screen, c, y1, y2, grid_color);
}

void Editor::flashCurrentLayer()
{
#if 0                           // TODO this flash effect can be done
                                // only with hardware acceleration.
                                // Finish it when the
                                // Allegro 5 port is ready.
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

    drawSpriteSafe(0, 0, m_sprite->getWidth()-1, m_sprite->getHeight()-1);
    gui_flip_screen();

    image_clear(flash_image, flash_image->mask_color);
    drawSpriteSafe(0, 0, m_sprite->getWidth()-1, m_sprite->getHeight()-1);
  }
#endif
}

void Editor::controlInfiniteScroll(Message* msg)
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

tools::Tool* Editor::getCurrentEditorTool()
{
  if (m_quicktool)
    return m_quicktool;
  else {
    UIContext* context = UIContext::instance();
    return context->getSettings()->getCurrentTool();
  }
}

void Editor::screenToEditor(int xin, int yin, int* xout, int* yout)
{
  View* view = View::getView(this);
  Rect vp = view->getViewportBounds();
  Point scroll = view->getViewScroll();

  *xout = (xin - vp.x + scroll.x - m_offset_x) >> m_zoom;
  *yout = (yin - vp.y + scroll.y - m_offset_y) >> m_zoom;
}

void Editor::screenToEditor(const Rect& in, Rect* out)
{
  int x1, y1, x2, y2;
  screenToEditor(in.x, in.y, &x1, &y1);
  screenToEditor(in.x+in.w, in.y+in.h, &x2, &y2);
  *out = Rect(x1, y1, x2 - x1, y2 - y1);
}

void Editor::editorToScreen(int xin, int yin, int* xout, int* yout)
{
  View* view = View::getView(this);
  Rect vp = view->getViewportBounds();
  Point scroll = view->getViewScroll();

  *xout = (vp.x - scroll.x + m_offset_x + (xin << m_zoom));
  *yout = (vp.y - scroll.y + m_offset_y + (yin << m_zoom));
}

void Editor::editorToScreen(const Rect& in, Rect* out)
{
  int x1, y1, x2, y2;
  editorToScreen(in.x, in.y, &x1, &y1);
  editorToScreen(in.x+in.w, in.y+in.h, &x2, &y2);
  *out = Rect(x1, y1, x2 - x1, y2 - y1);
}

void Editor::showDrawingCursor()
{
  ASSERT(m_sprite != NULL);

  if (!m_cursor_thick && canDraw()) {
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

void Editor::moveDrawingCursor()
{
  // Draw cursor
  if (m_cursor_thick) {
    int x, y;

    x = jmouse_x(0);
    y = jmouse_y(0);

    // Redraw it only when the mouse change to other pixel (not
    // when the mouse moves only).
    if ((m_cursor_screen_x != x) || (m_cursor_screen_y != y)) {
      jmouse_hide();
      editor_move_cursor(x, y);
      jmouse_show();
    }
  }
}

void Editor::addListener(EditorListener* listener)
{
  m_listeners.addListener(listener);
}

void Editor::removeListener(EditorListener* listener)
{
  m_listeners.removeListener(listener);
}

void Editor::setCustomizationDelegate(EditorCustomizationDelegate* delegate)
{
  if (m_customizationDelegate)
    m_customizationDelegate->dispose();

  m_customizationDelegate = delegate;
}

// Returns the visible area of the active sprite.
Rect Editor::getVisibleSpriteBounds()
{
  // Return an empty rectangle if there is not a active sprite.
  if (!m_sprite) return Rect();

  View* view = View::getView(this);
  Rect vp = view->getViewportBounds();
  int x1, y1, x2, y2;

  screenToEditor(vp.x, vp.y, &x1, &y1);
  screenToEditor(vp.x+vp.w-1, vp.y+vp.h-1, &x2, &y2);

  return Rect(x1, y1, x2-x1+1, y2-y1+1);
}

// Changes the scroll to see the given point as the center of the editor.
void Editor::centerInSpritePoint(int x, int y)
{
  View* view = View::getView(this);
  Rect vp = view->getViewportBounds();

  hideDrawingCursor();

  x = m_offset_x - (vp.w/2) + ((1<<m_zoom)>>1) + (x << m_zoom);
  y = m_offset_y - (vp.h/2) + ((1<<m_zoom)>>1) + (y << m_zoom);

  updateEditor();
  setEditorScroll(x, y, false);

  showDrawingCursor();
  invalidate();

  // Notify listeners
  m_listeners.notifyScrollChanged(this);
}

void Editor::updateStatusBar()
{
  // Setup status bar using the current editor's state
  m_state->onUpdateStatusBar(this);
}

void Editor::editor_update_quicktool()
{
  if (m_customizationDelegate) {
    UIContext* context = UIContext::instance();
    tools::Tool* current_tool = context->getSettings()->getCurrentTool();
    tools::Tool* old_quicktool = m_quicktool;

    m_quicktool = m_customizationDelegate->getQuickTool(current_tool);

    // If the tool has changed, we must to update the status bar because
    // the new tool can display something different in the status bar (e.g. Eyedropper)
    if (old_quicktool != m_quicktool)
      updateStatusBar();
  }
}

//////////////////////////////////////////////////////////////////////
// Message handler for the editor

bool Editor::onProcessMessage(Message* msg)
{
  switch (msg->type) {

    case JM_REQSIZE:
      editor_request_size(&msg->reqsize.w, &msg->reqsize.h);
      return true;

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
            m_mask_timer.start();
          }
          else {
            m_mask_timer.stop();
          }

          // Draw the cursor again
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
      if (msg->timer.timer == &m_mask_timer) {
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
      editor_update_quicktool();
      break;

    case JM_MOUSELEAVE:
      hideDrawingCursor();
      app_get_statusbar()->clearText();
      break;

    case JM_BUTTONPRESSED:
      if (m_sprite) {
        EditorStatePtr holdState(m_state);
        return m_state->onMouseDown(this, msg);
      }
      break;

    case JM_MOTION:
      if (m_sprite) {
        EditorStatePtr holdState(m_state);
        return m_state->onMouseMove(this, msg);
      }
      break;

    case JM_BUTTONRELEASED:
      if (m_sprite) {
        EditorStatePtr holdState(m_state);
        if (m_state->onMouseUp(this, msg))
          return true;
      }
      break;

    case JM_KEYPRESSED:
      if (m_sprite) {
        EditorStatePtr holdState(m_state);
        bool used = m_state->onKeyDown(this, msg);

        if (hasMouse()) {
          editor_update_quicktool();
          editor_setcursor();
        }

        if (used)
          return true;
      }
      break;

    case JM_KEYRELEASED:
      if (m_sprite) {
        EditorStatePtr holdState(m_state);
        bool used = m_state->onKeyUp(this, msg);

        if (hasMouse()) {
          editor_update_quicktool();
          editor_setcursor();
        }

        if (used)
          return true;
      }
      break;

    case JM_FOCUSLEAVE:
      // As we use keys like Space-bar as modifier, we can clear the
      // keyboard buffer when we lost the focus.
      clear_keybuf();
      break;

    case JM_WHEEL:
      if (m_sprite && hasMouse()) {
        EditorStatePtr holdState(m_state);
        if (m_state->onMouseWheel(this, msg))
          return true;
      }
      break;

    case JM_SETCURSOR:
      editor_setcursor();
      return true;
  }

  return Widget::onProcessMessage(msg);
}

// When the current tool is changed
void Editor::onCurrentToolChange()
{
  m_state->onCurrentToolChange(this);
}

void Editor::onFgColorChange()
{
  if (m_cursor_thick) {
    hideDrawingCursor();
    showDrawingCursor();
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

    m_offset_x = std::max<int>(vp.w/2, vp.w - m_sprite->getWidth()/2);
    m_offset_y = std::max<int>(vp.h/2, vp.h - m_sprite->getHeight()/2);

    *w = (m_sprite->getWidth() << m_zoom) + m_offset_x*2;
    *h = (m_sprite->getHeight() << m_zoom) + m_offset_y*2;
  }
  else {
    *w = 4;
    *h = 4;
  }
}

void Editor::editor_setcursor()
{
  bool used = false;
  if (m_sprite)
    used = m_state->onSetCursor(this);

  if (!used) {
    hideDrawingCursor();
    jmouse_set_cursor(JI_CURSOR_NORMAL);
  }
}

bool Editor::canDraw()
{
  return
    (m_sprite != NULL &&
     m_sprite->getCurrentLayer() != NULL &&
     m_sprite->getCurrentLayer()->is_image() &&
     m_sprite->getCurrentLayer()->is_readable() &&
     m_sprite->getCurrentLayer()->is_writable() /* && */
     /* layer_get_cel(m_sprite->layer, m_sprite->frame) != NULL */
     );
}

bool Editor::isInsideSelection()
{
  int x, y;
  screenToEditor(jmouse_x(0), jmouse_y(0), &x, &y);
  return
    m_document != NULL &&
    m_document->isMaskVisible() &&
    m_document->getMask()->containsPoint(x, y);
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

    // Notify listeners
    m_listeners.notifyScrollChanged(this);
  }
  showDrawingCursor();
}

void Editor::pasteImage(const Image* image, int x, int y)
{
  // Change to a selection tool: it's necessary for PixelsMovement
  // which will use the extra cel for transformation preview, and is
  // not compatible with the drawing cursor preview which overwrite
  // the extra cel.
  tools::Tool* currentTool = getCurrentEditorTool();
  if (!currentTool->getInk(0)->isSelection()) {
    tools::Tool* defaultSelectionTool =
      App::instance()->getToolBox()->getToolById(tools::WellKnownTools::RectangularMarquee);

    toolbar_select_tool(app_get_toolbar(), defaultSelectionTool);
  }

  Document* document = getDocument();
  int opacity = 255;
  Sprite* sprite = getSprite();

  // Check bounds where the image will be pasted.
  {
    // First we limit the image inside the sprite's bounds.
    x = MID(0, x, sprite->getWidth() - image->w);
    y = MID(0, y, sprite->getHeight() - image->h);

    // Then we check if the image will be visible by the user.
    Rect visibleBounds = getVisibleSpriteBounds();
    x = MID(visibleBounds.x-image->w, x, visibleBounds.x+visibleBounds.w-1);
    y = MID(visibleBounds.y-image->h, y, visibleBounds.y+visibleBounds.h-1);

    // If the visible part of the pasted image will not fit in the
    // visible bounds of the editor, we put the image in the center of
    // the visible bounds.
    Rect visiblePasted = visibleBounds.createIntersect(gfx::Rect(x, y, image->w, image->h));
    if (((visibleBounds.w >= image->w && visiblePasted.w < image->w/2) ||
         (visibleBounds.w <  image->w && visiblePasted.w < visibleBounds.w/2)) ||
        ((visibleBounds.h >= image->h && visiblePasted.h < image->w/2) ||
         (visibleBounds.h <  image->h && visiblePasted.h < visibleBounds.h/2))) {
      x = visibleBounds.x + visibleBounds.w/2 - image->w/2;
      y = visibleBounds.y + visibleBounds.h/2 - image->h/2;
    }
  }

  PixelsMovement* pixelsMovement =
    new PixelsMovement(document, sprite, image, x, y, opacity, "Paste");

  // Select the pasted image so the user can move it and transform it.
  pixelsMovement->maskImage(image, x, y);

  setState(EditorStatePtr(new MovingPixelsState(this, NULL, pixelsMovement, NoHandle)));
}
