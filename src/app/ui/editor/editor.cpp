/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/editor.h"

#include "app/app.h"
#include "app/color.h"
#include "app/color_utils.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/document_location.h"
#include "app/ini_file.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/settings/document_settings.h"
#include "app/settings/settings.h"
#include "app/tools/ink.h"
#include "app/tools/tool.h"
#include "app/tools/tool_box.h"
#include "app/ui/color_bar.h"
#include "app/ui/context_bar.h"
#include "app/ui/editor/editor_customization_delegate.h"
#include "app/ui/editor/editor_decorator.h"
#include "app/ui/editor/moving_pixels_state.h"
#include "app/ui/editor/pixels_movement.h"
#include "app/ui/editor/standby_state.h"
#include "app/ui/main_window.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui/toolbar.h"
#include "app/ui_context.h"
#include "app/util/boundary.h"
#include "app/util/misc.h"
#include "app/util/render.h"
#include "base/bind.h"
#include "base/unique_ptr.h"
#include "raster/conversion_alleg.h"
#include "raster/raster.h"
#include "ui/ui.h"

#include <allegro.h>
#include <stdio.h>

namespace app {

using namespace app::skin;
using namespace gfx;
using namespace ui;

class EditorPreRenderImpl : public EditorPreRender {
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
    blend_rect(m_image,
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

class EditorPostRenderImpl : public EditorPostRender {
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

Editor::Editor(Document* document)
  : Widget(editor_type())
  , m_state(new StandbyState())
  , m_decorator(NULL)
  , m_document(document)
  , m_sprite(m_document->getSprite())
  , m_layer(m_sprite->getFolder()->getFirstLayer())
  , m_frame(FrameNumber(0))
  , m_zoom(0)
  , m_mask_timer(100, this)
  , m_customizationDelegate(NULL)
  , m_docView(NULL)
{
  // Add the first state into the history.
  m_statesHistory.push(m_state);

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
    ColorBar::instance()->FgColorChange.connect(Bind<void>(&Editor::onFgColorChange, this));
}

Editor::~Editor()
{
  setCustomizationDelegate(NULL);

  m_mask_timer.stop();

  // Remove this editor as observer of CurrentToolChange signal.
  App::instance()->CurrentToolChange.disconnect(m_currentToolChangeSlot);
  delete m_currentToolChangeSlot;

  // Remove this editor as observer of FgColorChange
  ColorBar::instance()->FgColorChange.disconnect(m_fgColorChangeSlot);
  delete m_fgColorChangeSlot;
}

WidgetType editor_type()
{
  static WidgetType type = kGenericWidget;
  if (type == kGenericWidget)
    type = register_widget_type();
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

  // Notify observers
  m_observers.notifyStateChanged(this);

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

void Editor::setLayer(const Layer* layer)
{
  m_layer = const_cast<Layer*>(layer);
  m_observers.notifyLayerChanged(this);

  updateStatusBar();
}

void Editor::setFrame(FrameNumber frame)
{
  if (m_frame != frame) {
    m_frame = frame;
    m_observers.notifyFrameChanged(this);

    invalidate();
    updateStatusBar();
  }
}

void Editor::getDocumentLocation(DocumentLocation* location) const
{
  location->document(m_document);
  location->sprite(m_sprite);
  location->layer(m_layer);
  location->frame(m_frame);
}

DocumentLocation Editor::getDocumentLocation() const
{
  DocumentLocation location;
  getDocumentLocation(&location);
  return location;
}

void Editor::setDefaultScroll()
{
  View* view = View::getView(this);
  Rect vp = view->getViewportBounds();

  setEditorScroll(m_offset_x - vp.w/2 + (m_sprite->getWidth()/2),
                  m_offset_y - vp.h/2 + (m_sprite->getHeight()/2), false);
}

// Sets the scroll position of the editor
void Editor::setEditorScroll(int x, int y, int use_refresh_region)
{
  View* view = View::getView(this);
  Point oldScroll;
  Region region;
  int thick = m_cursor_thick;

  if (thick)
    editor_clean_cursor();

  if (use_refresh_region) {
    getDrawableRegion(region, kCutTopWindows);
    oldScroll = view->getViewScroll();
  }

  view->setViewScroll(Point(x, y));
  Point newScroll = view->getViewScroll();

  if (use_refresh_region) {
    // Move screen with blits
    scrollRegion(region,
                 oldScroll.x - newScroll.x,
                 oldScroll.y - newScroll.y);
  }

  if (thick)
    editor_draw_cursor(m_cursor_screen_x, m_cursor_screen_y);

  // Notify observers
  m_observers.notifyScrollChanged(this);
}

void Editor::updateEditor()
{
  View::getView(this)->updateView();
}

void Editor::drawSpriteUnclippedRect(const gfx::Rect& rc)
{
  View* view = View::getView(this);
  Rect vp = view->getViewportBounds();
  int source_x, source_y, dest_x, dest_y, width, height;

  // Get scroll

  Point scroll = view->getViewScroll();

  // Output information

  source_x = rc.x << m_zoom;
  source_y = rc.y << m_zoom;
  dest_x   = vp.x - scroll.x + m_offset_x + source_x;
  dest_y   = vp.y - scroll.y + m_offset_y + source_y;
  width    = rc.w << m_zoom;
  height   = rc.h << m_zoom;

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
    RenderEngine renderEngine(m_document, m_sprite, m_layer, m_frame);

    // Generate the rendered image
    base::UniquePtr<Image> rendered
      (renderEngine.renderSprite(source_x, source_y, width, height,
                                 m_frame, m_zoom, true));

    if (rendered) {
      // Pre-render decorator.
      if (m_decorator) {
        EditorPreRenderImpl preRender(this, rendered,
                                      Point(-source_x, -source_y), m_zoom);
        m_decorator->preRenderDecorator(&preRender);
      }

#ifdef DRAWSPRITE_DOUBLEBUFFERED
      BITMAP *bmp = create_bitmap(width, height);

      image_to_allegro(rendered, bmp, 0, 0, m_sprite->getPalette(m_frame));
      blit(bmp, ji_screen, 0, 0, dest_x, dest_y, width, height);

      destroy_bitmap(bmp);
#else
      acquire_bitmap(ji_screen);
      convert_image_to_allegro(rendered, ji_screen, dest_x, dest_y,
                               m_sprite->getPalette(m_frame));
      release_bitmap(ji_screen);
#endif
    }
  }

  // Draw grids
  IDocumentSettings* docSettings =
      UIContext::instance()->getSettings()->getDocumentSettings(m_document);

  // Draw the pixel grid
  if (docSettings->getPixelGridVisible()) {
    if (m_zoom > 1)
      this->drawGrid(Rect(0, 0, 1, 1),
                     docSettings->getPixelGridColor());
  }

  // Draw the grid
  if (docSettings->getGridVisible())
    this->drawGrid(docSettings->getGridBounds(),
                   docSettings->getGridColor());

  // Draw the mask
  if (m_document->getBoundariesSegments())
    this->drawMask();

  // Post-render decorator.
  if (m_decorator) {
    EditorPostRenderImpl postRender(this);
    m_decorator->postRenderDecorator(&postRender);
  }
}

void Editor::drawSpriteClipped(const gfx::Region& updateRegion)
{
  Region region;
  getDrawableRegion(region, kCutTopWindows);

  int cx1, cy1, cx2, cy2;
  get_clip_rect(ji_screen, &cx1, &cy1, &cx2, &cy2);

  for (Region::const_iterator
         it=region.begin(), end=region.end(); it != end; ++it) {
    const Rect& rc = *it;

    add_clip_rect(ji_screen, rc.x, rc.y, rc.x2()-1, rc.y2()-1);

    for (Region::const_iterator
           it2=updateRegion.begin(), end2=updateRegion.end(); it2 != end2; ++it2) {
      drawSpriteUnclippedRect(*it2);
    }

    set_clip_rect(ji_screen, cx1, cy1, cx2, cy2);
  }
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
  const BoundSeg* seg = m_document->getBoundariesSegments();

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

    Region region;
    getDrawableRegion(region, kCutTopWindows);

    acquire_bitmap(ji_screen);

    if (thick)
      editor_clean_cursor();
    else
      jmouse_hide();

    for (Region::const_iterator it=region.begin(), end=region.end();
         it != end; ++it) {
      const Rect& rc = *it;
      set_clip_rect(ji_screen, rc.x, rc.y, rc.x2()-1, rc.y2()-1);
      drawMask();
    }
    set_clip_rect(ji_screen, 0, 0, JI_SCREEN_W-1, JI_SCREEN_H-1);

    // Draw the cursor
    if (thick)
      editor_draw_cursor(m_cursor_screen_x, m_cursor_screen_y);
    else
      jmouse_show();

    release_bitmap(ji_screen);
  }
}

void Editor::drawGrid(const Rect& gridBounds, const app::Color& color)
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

    clear_image(flash_image, flash_image->mask_color);
    for (v=0; v<flash_image->getHeight(); ++v) {
      for (u=0; u<flash_image->getWidth(); ++u) {
        if (u-x >= 0 && u-x < src_image->getWidth() &&
            v-y >= 0 && v-y < src_image->getHeight()) {
          uint32_t color = get_pixel(src_image, u-x, v-y);
          if (color != src_image->mask_color) {
            Color ccc = Color::fromRgb(255, 255, 255);
            put_pixel(flash_image, u, v,
                      color_utils::color_for_image(ccc, flash_image->imgtype));
          }
        }
      }
    }

    drawSpriteSafe(0, 0, m_sprite->getWidth()-1, m_sprite->getHeight()-1);
    gui_flip_screen();

    clear_image(flash_image, flash_image->mask_color);
    drawSpriteSafe(0, 0, m_sprite->getWidth()-1, m_sprite->getHeight()-1);
  }
#endif
}

gfx::Point Editor::controlInfiniteScroll(MouseMessage* msg)
{
  View* view = View::getView(this);
  gfx::Rect vp = view->getViewportBounds();
  gfx::Point mousePos = msg->position();

  gfx::Point delta = ui::get_delta_outside_box(vp, mousePos);
  if (delta != gfx::Point(0, 0)) {
    // Scrolling-by-steps (non-smooth), this is better for high
    // resolutions: scroll movement by big steps.
    if (!get_config_bool("Options", "MoveSmooth", true)) {
      gfx::Point newPos = mousePos;
      if (delta.x != 0) newPos.x = (mousePos.x-delta.x+(vp.x+vp.w/2))/2;
      if (delta.y != 0) newPos.y = (mousePos.y-delta.y+(vp.y+vp.h/2))/2;
      delta = mousePos - newPos;
    }

    mousePos.x -= delta.x;
    mousePos.y -= delta.y;
    ui::set_mouse_position(mousePos);

    gfx::Point scroll = view->getViewScroll();
    scroll += delta;
    setEditorScroll(scroll.x, scroll.y, true);
  }

  return mousePos;
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

void Editor::addObserver(EditorObserver* observer)
{
  m_observers.addObserver(observer);
}

void Editor::removeObserver(EditorObserver* observer)
{
  m_observers.removeObserver(observer);
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

  // Notify observers
  m_observers.notifyScrollChanged(this);
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
    if (old_quicktool != m_quicktool) {
      updateStatusBar();

      App::instance()->getMainWindow()->getContextBar()
        ->updateFromTool(getCurrentEditorTool());
    }
  }
}

//////////////////////////////////////////////////////////////////////
// Message handler for the editor

bool Editor::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kPaintMessage: {
      SkinTheme* theme = static_cast<SkinTheme*>(this->getTheme());

      int old_cursor_thick = m_cursor_thick;
      if (m_cursor_thick)
        editor_clean_cursor();

      // Editor without sprite
      if (!m_sprite) {
        View* view = View::getView(this);
        Rect vp = view->getViewportBounds();

        jdraw_rectfill(vp, theme->getColor(ThemeColor::EditorFace));
        draw_emptyset_symbol(ji_screen, vp, ui::rgba(64, 64, 64));
      }
      // Editor with sprite
      else {
        try {
          // Lock the sprite to read/render it.
          DocumentReader documentReader(m_document);
          int x1, y1, x2, y2;

          // Draw the background outside of sprite's bounds
          x1 = getBounds().x + m_offset_x;
          y1 = getBounds().y + m_offset_y;
          x2 = x1 + (m_sprite->getWidth() << m_zoom) - 1;
          y2 = y1 + (m_sprite->getHeight() << m_zoom) - 1;

          jdraw_rectexclude(getBounds(),
                            gfx::Rect(gfx::Point(x1-1, y1-1),
                                      gfx::Point(x2+1, y2+2)),
                            theme->getColor(ThemeColor::EditorFace));

          // Draw the sprite in the editor
          drawSpriteUnclippedRect(gfx::Rect(0, 0, m_sprite->getWidth(), m_sprite->getHeight()));

          // Draw the sprite boundary
          rect(ji_screen, x1-1, y1-1, x2+1, y2+1, to_system(theme->getColor(ThemeColor::EditorSpriteBorder)));
          hline(ji_screen, x1-1, y2+2, x2+1, to_system(theme->getColor(ThemeColor::EditorSpriteBottomBorder)));

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
          jdraw_rectfill(vp, theme->getColor(ThemeColor::EditorFace));
        }
      }
      return true;
    }

    case kTimerMessage:
      if (static_cast<TimerMessage*>(msg)->timer() == &m_mask_timer) {
        if (isVisible() && m_sprite) {
          drawMaskSafe();

          // Set offset to make selection-movement effect
          if (m_offset_count < 7)
            m_offset_count++;
          else
            m_offset_count = 0;
        }
        else if (m_mask_timer.isRunning()) {
          m_mask_timer.stop();
        }
      }
      break;

    case kMouseEnterMessage:
      editor_update_quicktool();
      break;

    case kMouseLeaveMessage:
      hideDrawingCursor();
      StatusBar::instance()->clearText();
      break;

    case kMouseDownMessage:
      if (m_sprite) {
        m_oldPos = static_cast<MouseMessage*>(msg)->position();

        EditorStatePtr holdState(m_state);
        return m_state->onMouseDown(this, static_cast<MouseMessage*>(msg));
      }
      break;

    case kMouseMoveMessage:
      if (m_sprite) {
        EditorStatePtr holdState(m_state);
        return m_state->onMouseMove(this, static_cast<MouseMessage*>(msg));
      }
      break;

    case kMouseUpMessage:
      if (m_sprite) {
        EditorStatePtr holdState(m_state);
        if (m_state->onMouseUp(this, static_cast<MouseMessage*>(msg)))
          return true;
      }
      break;

    case kKeyDownMessage:
      if (m_sprite) {
        EditorStatePtr holdState(m_state);
        bool used = m_state->onKeyDown(this, static_cast<KeyMessage*>(msg));

        if (hasMouse()) {
          editor_update_quicktool();
          editor_setcursor();
        }

        if (used)
          return true;
      }
      break;

    case kKeyUpMessage:
      if (m_sprite) {
        EditorStatePtr holdState(m_state);
        bool used = m_state->onKeyUp(this, static_cast<KeyMessage*>(msg));

        if (hasMouse()) {
          editor_update_quicktool();
          editor_setcursor();
        }

        if (used)
          return true;
      }
      break;

    case kFocusLeaveMessage:
      // As we use keys like Space-bar as modifier, we can clear the
      // keyboard buffer when we lost the focus.
      clear_keybuf();
      break;

    case kMouseWheelMessage:
      if (m_sprite && hasMouse()) {
        EditorStatePtr holdState(m_state);
        if (m_state->onMouseWheel(this, static_cast<MouseMessage*>(msg)))
          return true;
      }
      break;

    case kSetCursorMessage:
      editor_setcursor();
      return true;
  }

  return Widget::onProcessMessage(msg);
}

void Editor::onPreferredSize(PreferredSizeEvent& ev)
{
  gfx::Size sz(0, 0);

  if (m_sprite) {
    View* view = View::getView(this);
    Rect vp = view->getViewportBounds();

    m_offset_x = std::max<int>(vp.w/2, vp.w - m_sprite->getWidth()/2);
    m_offset_y = std::max<int>(vp.h/2, vp.h - m_sprite->getHeight()/2);

    sz.w = (m_sprite->getWidth() << m_zoom) + m_offset_x*2;
    sz.h = (m_sprite->getHeight() << m_zoom) + m_offset_y*2;
  }
  else {
    sz.w = 4;
    sz.h = 4;
  }
  ev.setPreferredSize(sz);
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

void Editor::editor_setcursor()
{
  bool used = false;
  if (m_sprite)
    used = m_state->onSetCursor(this);

  if (!used) {
    hideDrawingCursor();
    jmouse_set_cursor(kArrowCursor);
  }
}

bool Editor::canDraw()
{
  return (m_layer != NULL &&
          m_layer->isImage() &&
          m_layer->isReadable() &&
          m_layer->isWritable());
}

bool Editor::isInsideSelection()
{
  int x, y;
  screenToEditor(jmouse_x(0), jmouse_y(0), &x, &y);
  return
    (UIContext::instance()->settings()->selection()->getSelectionMode() != kSubtractSelectionMode) &&
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
      ui::set_mouse_position(gfx::Point(mx, my));

    // Notify observers
    m_observers.notifyScrollChanged(this);
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

    ToolBar::instance()->selectTool(defaultSelectionTool);
  }

  Document* document = getDocument();
  int opacity = 255;
  Sprite* sprite = getSprite();
  Layer* layer = getLayer();

  // Check bounds where the image will be pasted.
  {
    // First we limit the image inside the sprite's bounds.
    x = MID(0, x, sprite->getWidth() - image->getWidth());
    y = MID(0, y, sprite->getHeight() - image->getHeight());

    // Then we check if the image will be visible by the user.
    Rect visibleBounds = getVisibleSpriteBounds();
    x = MID(visibleBounds.x-image->getWidth(), x, visibleBounds.x+visibleBounds.w-1);
    y = MID(visibleBounds.y-image->getHeight(), y, visibleBounds.y+visibleBounds.h-1);

    // If the visible part of the pasted image will not fit in the
    // visible bounds of the editor, we put the image in the center of
    // the visible bounds.
    Rect visiblePasted = visibleBounds.createIntersect(gfx::Rect(x, y, image->getWidth(), image->getHeight()));
    if (((visibleBounds.w >= image->getWidth() && visiblePasted.w < image->getWidth()/2) ||
         (visibleBounds.w <  image->getWidth() && visiblePasted.w < visibleBounds.w/2)) ||
        ((visibleBounds.h >= image->getHeight() && visiblePasted.h < image->getWidth()/2) ||
         (visibleBounds.h <  image->getHeight() && visiblePasted.h < visibleBounds.h/2))) {
      x = visibleBounds.x + visibleBounds.w/2 - image->getWidth()/2;
      y = visibleBounds.y + visibleBounds.h/2 - image->getHeight()/2;
    }
  }

  PixelsMovementPtr pixelsMovement(
    new PixelsMovement(UIContext::instance(),
      document, sprite, layer,
      image, x, y, opacity, "Paste"));

  // Select the pasted image so the user can move it and transform it.
  pixelsMovement->maskImage(image, x, y);

  setState(EditorStatePtr(new MovingPixelsState(this, NULL, pixelsMovement, NoHandle)));
}

} // namespace app
