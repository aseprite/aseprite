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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/editor.h"

#include "app/app.h"
#include "app/color.h"
#include "app/color_utils.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/console.h"
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
#include "raster/conversion_she.h"
#include "raster/raster.h"
#include "she/surface.h"
#include "she/system.h"
#include "ui/ui.h"

#include <allegro.h>
#include <cstdio>

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

  Editor* getEditor() override
  {
    return m_editor;
  }

  Image* getImage() override
  {
    return m_image;
  }

  void fillRect(const gfx::Rect& rect, uint32_t rgbaColor, int opacity) override
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
  EditorPostRenderImpl(Editor* editor, Graphics* g)
    : m_editor(editor)
    , m_g(g)
  {
  }

  Editor* getEditor()
  {
    return m_editor;
  }

  void drawLine(int x1, int y1, int x2, int y2, gfx::Color screenColor)
  {
    int u1, v1, u2, v2;
    m_editor->editorToScreen(x1, y1, &u1, &v1);
    m_editor->editorToScreen(x2, y2, &u2, &v2);
    gfx::Rect bounds = m_editor->getBounds();
    u1 -= bounds.x;
    v1 -= bounds.y;
    u2 -= bounds.x;
    v2 -= bounds.y;
    m_g->drawLine(screenColor, gfx::Point(u1, v1), gfx::Point(u2, v2));
  }

private:
  Editor* m_editor;
  Graphics* m_g;
};

static raster::ImageBufferPtr render_buffer;

Editor::Editor(Document* document, EditorFlags flags)
  : Widget(editor_type())
  , m_state(new StandbyState())
  , m_decorator(NULL)
  , m_document(document)
  , m_sprite(m_document->sprite())
  , m_layer(m_sprite->folder()->getFirstLayer())
  , m_frame(FrameNumber(0))
  , m_zoom(0)
  , m_selectionMode(kDefaultSelectionMode)
  , m_offset_x(0)
  , m_offset_y(0)
  , m_mask_timer(100, this)
  , m_offset_count(0)
  , m_customizationDelegate(NULL)
  , m_docView(NULL)
  , m_flags(flags)
  , m_secondaryButton(false)
{
  // Add the first state into the history.
  m_statesHistory.push(m_state);

  m_cursor_thick = 0;
  m_cursor_screen_x = 0;
  m_cursor_screen_y = 0;
  m_cursor_editor_x = 0;
  m_cursor_editor_y = 0;

  m_quicktool = NULL;

  this->setFocusStop(true);

  m_currentToolChangeConn =
    App::instance()->CurrentToolChange.connect(&Editor::onCurrentToolChange, this);

  m_fgColorChangeConn =
    ColorBar::instance()->FgColorChange.connect(Bind<void>(&Editor::onFgColorChange, this));

  UIContext::instance()->settings()
    ->getDocumentSettings(m_document)
    ->addObserver(this);

  m_state->onAfterChangeState(this);
}

Editor::~Editor()
{
  UIContext::instance()->settings()
    ->getDocumentSettings(m_document)
    ->removeObserver(this);

  setCustomizationDelegate(NULL);

  m_mask_timer.stop();
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
  m_observers.notifyBeforeLayerChanged(this);
  m_layer = const_cast<Layer*>(layer);
  m_observers.notifyAfterLayerChanged(this);

  updateStatusBar();
}

void Editor::setFrame(FrameNumber frame)
{
  if (m_frame != frame) {
    m_observers.notifyBeforeFrameChanged(this);
    m_frame = frame;
    m_observers.notifyAfterFrameChanged(this);

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

  setEditorScroll(m_offset_x - vp.w/2 + (m_sprite->width()/2),
                  m_offset_y - vp.h/2 + (m_sprite->height()/2), false);
}

// Sets the scroll position of the editor
void Editor::setEditorScroll(int x, int y, bool blit_valid_rgn)
{
  View* view = View::getView(this);
  Point oldScroll;
  Region region;
  int thick = m_cursor_thick;

  if (thick)
    clearBrushPreview();

  if (blit_valid_rgn) {
    getDrawableRegion(region, kCutTopWindows);
    oldScroll = view->getViewScroll();
  }

  view->setViewScroll(Point(x, y));
  Point newScroll = view->getViewScroll();

  if (blit_valid_rgn) {
    // Move screen with blits
    scrollRegion(region,
                 oldScroll.x - newScroll.x,
                 oldScroll.y - newScroll.y);
  }

  if (thick)
    drawBrushPreview(m_cursor_screen_x, m_cursor_screen_y);
}

void Editor::setEditorZoom(int zoom)
{
  setZoomAndCenterInMouse(zoom,
    jmouse_x(0), jmouse_y(0),
    Editor::kCofiguredZoomBehavior);
}

void Editor::updateEditor()
{
  View::getView(this)->updateView();
}

void Editor::drawOneSpriteUnclippedRect(ui::Graphics* g, const gfx::Rect& rc, int dx, int dy)
{
  // Output information
  int source_x = rc.x << m_zoom;
  int source_y = rc.y << m_zoom;
  int dest_x   = dx + m_offset_x + source_x;
  int dest_y   = dy + m_offset_y + source_y;
  int width    = rc.w << m_zoom;
  int height   = rc.h << m_zoom;

  // Clip from graphics/screen
  const gfx::Rect& clip = g->getClipBounds();
  if (dest_x < clip.x) {
    source_x += clip.x - dest_x;
    width -= clip.x - dest_x;
    dest_x = clip.x;
  }
  if (dest_y < clip.y) {
    source_y += clip.y - dest_y;
    height -= clip.y - dest_y;
    dest_y = clip.y;
  }
  if (dest_x+width > clip.x+clip.w) {
    width = clip.x+clip.w-dest_x;
  }
  if (dest_y+height > clip.y+clip.h) {
    height = clip.y+clip.h-dest_y;
  }

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
  if (source_x+width > (m_sprite->width() << m_zoom)) {
    width = (m_sprite->width() << m_zoom) - source_x;
  }
  if (source_y+height > (m_sprite->height() << m_zoom)) {
    height = (m_sprite->height() << m_zoom) - source_y;
  }

  // Draw the sprite
  if ((width > 0) && (height > 0)) {
    RenderEngine renderEngine(m_document, m_sprite, m_layer, m_frame);

    // Generate the rendered image
    if (!render_buffer)
      render_buffer.reset(new raster::ImageBuffer());

    base::UniquePtr<Image> rendered(NULL);
    try {
      rendered.reset(renderEngine.renderSprite(
          source_x, source_y, width, height,
          m_frame, m_zoom, true,
          ((m_flags & kShowOnionskin) == kShowOnionskin),
          render_buffer));
    }
    catch (const std::exception& e) {
      Console::showException(e);
    }

    if (rendered) {
      // Pre-render decorator.
      if ((m_flags & kShowDecorators) && m_decorator) {
        EditorPreRenderImpl preRender(this, rendered,
                                      Point(-source_x, -source_y), m_zoom);
        m_decorator->preRenderDecorator(&preRender);
      }

      she::Surface* tmp(she::instance()->createRgbaSurface(width, height));
      if (tmp->nativeHandle()) {
        convert_image_to_surface(rendered, m_sprite->getPalette(m_frame),
          tmp, 0, 0, 0, 0, width, height);
        g->blit(tmp, 0, 0, dest_x, dest_y, width, height);
      }
      tmp->dispose();
    }
  }
}

void Editor::drawSpriteUnclippedRect(ui::Graphics* g, const gfx::Rect& rc)
{
  gfx::Rect client = getClientBounds();
  gfx::Rect spriteRect(
    client.x + m_offset_x,
    client.y + m_offset_y,
    (m_sprite->width() << m_zoom),
    (m_sprite->height() << m_zoom));
  gfx::Rect enclosingRect = spriteRect;

  // Draw the main sprite at the center.
  drawOneSpriteUnclippedRect(g, rc, 0, 0);

  gfx::Region outside(client);
  outside.createSubtraction(outside, gfx::Region(spriteRect));

  // Document settings
  IDocumentSettings* docSettings =
      UIContext::instance()->settings()->getDocumentSettings(m_document);

  if (docSettings->getTiledMode() & filters::TILED_X_AXIS) {
    drawOneSpriteUnclippedRect(g, rc, -spriteRect.w, 0);
    drawOneSpriteUnclippedRect(g, rc, +spriteRect.w, 0);

    enclosingRect = gfx::Rect(spriteRect.x-spriteRect.w, spriteRect.y, spriteRect.w*3, spriteRect.h);
    outside.createSubtraction(outside, gfx::Region(enclosingRect));
  }

  if (docSettings->getTiledMode() & filters::TILED_Y_AXIS) {
    drawOneSpriteUnclippedRect(g, rc, 0, -spriteRect.h);
    drawOneSpriteUnclippedRect(g, rc, 0, +spriteRect.h);

    enclosingRect = gfx::Rect(spriteRect.x, spriteRect.y-spriteRect.h, spriteRect.w, spriteRect.h*3);
    outside.createSubtraction(outside, gfx::Region(enclosingRect));
  }

  if (docSettings->getTiledMode() == filters::TILED_BOTH) {
    drawOneSpriteUnclippedRect(g, rc, -spriteRect.w, -spriteRect.h);
    drawOneSpriteUnclippedRect(g, rc, +spriteRect.w, -spriteRect.h);
    drawOneSpriteUnclippedRect(g, rc, -spriteRect.w, +spriteRect.h);
    drawOneSpriteUnclippedRect(g, rc, +spriteRect.w, +spriteRect.h);

    enclosingRect = gfx::Rect(
      spriteRect.x-spriteRect.w,
      spriteRect.y-spriteRect.h, spriteRect.w*3, spriteRect.h*3);
    outside.createSubtraction(outside, gfx::Region(enclosingRect));
  }

  // Fill the outside (parts of the editor that aren't covered by the
  // sprite).
  SkinTheme* theme = static_cast<SkinTheme*>(this->getTheme());
  if (m_flags & kShowOutside) {
    g->fillRegion(theme->getColor(ThemeColor::EditorFace), outside);
  }

  // Draw the pixel grid
  if ((m_zoom > 1) && docSettings->getPixelGridVisible()) {
    drawGrid(g, enclosingRect, Rect(0, 0, 1, 1), docSettings->getPixelGridColor());
  }

  // Draw the grid
  if (docSettings->getGridVisible())
    drawGrid(g, enclosingRect, docSettings->getGridBounds(), docSettings->getGridColor());

  if (m_flags & kShowOutside) {
    // Draw the borders that enclose the sprite.
    enclosingRect.enlarge(1);
    g->drawRect(theme->getColor(ThemeColor::EditorSpriteBorder), enclosingRect);
    g->drawHLine(
      theme->getColor(ThemeColor::EditorSpriteBottomBorder),
      enclosingRect.x, enclosingRect.y+enclosingRect.h, enclosingRect.w);
  }

  // Draw the mask
  if (m_document->getBoundariesSegments())
    drawMask(g);

  // Post-render decorator.
  if ((m_flags & kShowDecorators) && m_decorator) {
    EditorPostRenderImpl postRender(this, g);
    m_decorator->postRenderDecorator(&postRender);
  }
}

void Editor::drawSpriteClipped(const gfx::Region& updateRegion)
{
  Region region;
  getDrawableRegion(region, kCutTopWindows);

  ScreenGraphics g;

  for (Region::const_iterator
         it=region.begin(), end=region.end(); it != end; ++it) {
    const Rect& rc = *it;

    IntersectClip clip(&g, rc);
    if (clip) {
      for (Region::const_iterator
             it2=updateRegion.begin(), end2=updateRegion.end(); it2 != end2; ++it2) {
        drawSpriteUnclippedRect(getGraphics(getClientBounds()), *it2);
      }
    }
  }
}

/**
 * Draws the boundaries, really this routine doesn't use the "mask"
 * field of the sprite, only the "bound" field (so you can have other
 * mask in the sprite and could be showed other boundaries), to
 * regenerate boundaries, use the sprite_generate_mask_boundaries()
 * routine.
 */
void Editor::drawMask(Graphics* g)
{
  if ((m_flags & kShowMask) == 0)
    return;

  int x1, y1, x2, y2;
  int x = m_offset_x;
  int y = m_offset_y;

  int nseg = m_document->getBoundariesSegmentsCount();
  const BoundSeg* seg = m_document->getBoundariesSegments();

  dotted_mode(m_offset_count);

  for (int c=0; c<nseg; ++c, ++seg) {
    x1 = seg->x1 << m_zoom;
    y1 = seg->y1 << m_zoom;
    x2 = seg->x2 << m_zoom;
    y2 = seg->y2 << m_zoom;

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
    else {
      if (x1 == x2) {
        y2--;
      }
      else {
        x2--;
      }
    }

    // The color doesn't matter, we are using dotted_mode()
    // TODO send dotted_mode() to ui::Graphics domain.
    g->drawLine(0, gfx::Point(x+x1, y+y1), gfx::Point(x+x2, y+y2));
  }

  dotted_mode(-1);
}

void Editor::drawMaskSafe()
{
  if ((m_flags & kShowMask) == 0)
    return;

  if (isVisible() &&
      m_document &&
      m_document->getBoundariesSegments()) {
    int thick = m_cursor_thick;

    Region region;
    getDrawableRegion(region, kCutTopWindows);
    region.offset(-getBounds().getOrigin());

    if (thick)
      clearBrushPreview();
    else
      jmouse_hide();

    GraphicsPtr g = getGraphics(getClientBounds());

    for (Region::const_iterator it=region.begin(), end=region.end();
         it != end; ++it) {
      IntersectClip clip(g, gfx::Rect(*it));
      if (clip)
        drawMask(g);
    }

    // Draw the cursor
    if (thick)
      drawBrushPreview(m_cursor_screen_x, m_cursor_screen_y);
    else
      jmouse_show();
  }
}

void Editor::drawGrid(Graphics* g, const gfx::Rect& spriteBounds, const Rect& gridBounds, const app::Color& color)
{
  if ((m_flags & kShowGrid) == 0)
    return;

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

  // Adjust for client area
  gfx::Rect bounds = getBounds();
  grid.offset(-bounds.getOrigin());

  while (grid.x-grid.w >= spriteBounds.x) grid.x -= grid.w;
  while (grid.y-grid.h >= spriteBounds.y) grid.y -= grid.h;

  // Get the grid's color
  gfx::Color grid_color = color_utils::color_for_ui(color);

  // Draw horizontal lines
  int x1 = spriteBounds.x;
  int y1 = grid.y;
  int x2 = spriteBounds.x + spriteBounds.w;
  int y2 = spriteBounds.y + spriteBounds.h;

  for (int c=y1; c<=y2; c+=grid.h)
    g->drawHLine(grid_color, x1, c, spriteBounds.w);

  // Draw vertical lines
  x1 = grid.x;
  y1 = spriteBounds.y;

  for (int c=x1; c<=x2; c+=grid.w)
    g->drawVLine(grid_color, c, y1, spriteBounds.h);
}

void Editor::flashCurrentLayer()
{
  if (!UIContext::instance()->settings()->experimental()->flashLayer())
    return;

  DocumentLocation loc = getDocumentLocation();

  int x, y;
  const Image* src_image = loc.image(&x, &y);
  if (src_image) {
    RenderEngine::setPreviewImage(NULL, FrameNumber(0), NULL);

    m_document->prepareExtraCel(0, 0, m_sprite->width(), m_sprite->height(), 255);
    Image* flash_image = m_document->getExtraCelImage();

    clear_image(flash_image, flash_image->maskColor());
    copy_image(flash_image, src_image, x, y);
    m_document->setExtraCelBlendMode(BLEND_MODE_BLACKANDWHITE);

    drawSpriteClipped(gfx::Region(
        gfx::Rect(0, 0, m_sprite->width(), m_sprite->height())));
    gui_feedback();

    m_document->setExtraCelBlendMode(BLEND_MODE_NORMAL);
    m_document->destroyExtraCel();
    invalidate();
  }
}

gfx::Point Editor::autoScroll(MouseMessage* msg, AutoScroll dir, bool blit_valid_rgn)
{
  View* view = View::getView(this);
  gfx::Rect vp = view->getViewportBounds();
  gfx::Point mousePos = msg->position();

  if (!vp.contains(mousePos)) {
    gfx::Point delta = (mousePos - m_oldPos);
    gfx::Point deltaScroll = delta;

    if (!((mousePos.x <  vp.x      && delta.x < 0) ||
          (mousePos.x >= vp.x+vp.w && delta.x > 0))) {
      delta.x = 0;
    }

    if (!((mousePos.y <  vp.y      && delta.y < 0) ||
          (mousePos.y >= vp.y+vp.h && delta.y > 0))) {
      delta.y = 0;
    }

    gfx::Point scroll = view->getViewScroll();
    if (dir == AutoScroll::MouseDir) {
      scroll += delta;
    }
    else {
      scroll -= deltaScroll;
    }
    setEditorScroll(scroll.x, scroll.y, blit_valid_rgn);

#ifdef WIN32
    mousePos -= delta;
    ui::set_mouse_position(mousePos);
#endif

    m_oldPos = mousePos;
    mousePos = gfx::Point(
      MID(vp.x, mousePos.x, vp.x+vp.w-1),
      MID(vp.y, mousePos.y, vp.y+vp.h-1));
  }
  else
    m_oldPos = mousePos;

  return mousePos;
}

bool Editor::isCurrentToolAffectedByRightClickMode()
{
  Context* context = UIContext::instance();
  tools::Tool* tool = context->settings()->getCurrentTool();

  return
    (tool->getInk(0)->isPaint() || tool->getInk(0)->isEffect()) &&
    (!tool->getInk(0)->isEraser());
}

tools::Tool* Editor::getCurrentEditorTool()
{
  if (m_quicktool)
    return m_quicktool;

  Context* context = UIContext::instance();
  tools::Tool* tool = context->settings()->getCurrentTool();

  if (m_secondaryButton && isCurrentToolAffectedByRightClickMode()) {
    tools::ToolBox* toolbox = App::instance()->getToolBox();

    switch (context->settings()->getRightClickMode()) {
      case RightClickMode::PaintBgColor:
        // Do nothing, use the current tool
        break;
      case RightClickMode::PickFgColor:
        tool = toolbox->getToolById(tools::WellKnownTools::Eyedropper);
        break;
      case RightClickMode::Erase:
        tool = toolbox->getToolById(tools::WellKnownTools::Eraser);
        break;
    }
  }

  return tool;
}

tools::Ink* Editor::getCurrentEditorInk()
{
  Context* context = UIContext::instance();
  tools::Tool* tool = getCurrentEditorTool();
  tools::Ink* ink = tool->getInk(m_secondaryButton ? 1: 0);

  if (m_quicktool)
    return ink;

  RightClickMode rightClickMode = context->settings()->getRightClickMode();

  if (m_secondaryButton &&
      rightClickMode != RightClickMode::Default &&
      isCurrentToolAffectedByRightClickMode()) {
    tools::ToolBox* toolbox = App::instance()->getToolBox();

    switch (rightClickMode) {
      case RightClickMode::Default:
        // Do nothing
        break;
      case RightClickMode::PickFgColor:
        ink = toolbox->getInkById(tools::WellKnownInks::PickFg);
        break;
      case RightClickMode::Erase:
        ink = toolbox->getInkById(tools::WellKnownInks::Eraser);
        break;
    }
  }
  else {
    IToolSettings* toolSettings = context->settings()->getToolSettings(tool);
    InkType inkType = toolSettings->getInkType();
    const char* id = NULL;

    switch (inkType) {
      case kDefaultInk:
        // Do nothing
        break;
      case kOpaqueInk:
        id = tools::WellKnownInks::PaintOpaque;
        break;
      case kSetAlphaInk:
        id = tools::WellKnownInks::PaintSetAlpha;
        break;
      case kLockAlphaInk:
        id = tools::WellKnownInks::PaintLockAlpha;
        break;
      case kMergeInk:
        id = tools::WellKnownInks::Paint;
        break;
      case kShadingInk:
        id = tools::WellKnownInks::Shading;
        break;
      case kReplaceInk:
        if (!m_secondaryButton)
          id = tools::WellKnownInks::ReplaceBgWithFg;
        else
          id = tools::WellKnownInks::ReplaceFgWithBg;
        break;
      case kEraseInk:
        id = tools::WellKnownInks::Eraser;
        break;
      case kSelectionInk:
        id = tools::WellKnownInks::Selection;
        break;
      case kBlurInk:
        id = tools::WellKnownInks::Blur;
        break;
      case kJumbleInk:
        id = tools::WellKnownInks::Jumble;
        break;
    }

    if (id)
      ink = App::instance()->getToolBox()->getInkById(id);
  }

  return ink;
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
    drawBrushPreview(jmouse_x(0), jmouse_y(0));
    jmouse_show();
  }
}

void Editor::hideDrawingCursor()
{
  if (m_cursor_thick) {
    jmouse_hide();
    clearBrushPreview();
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
      moveBrushPreview(x, y);
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

  return Rect(0, 0, m_sprite->width(), m_sprite->height())
    .createIntersect(Rect(x1, y1, x2-x1+1, y2-y1+1));
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
}

void Editor::updateStatusBar()
{
  // Setup status bar using the current editor's state
  m_state->onUpdateStatusBar(this);
}

void Editor::updateQuicktool()
{
  if (m_customizationDelegate) {
    UIContext* context = UIContext::instance();
    tools::Tool* current_tool = context->settings()->getCurrentTool();

    // Don't change quicktools if we are in a selection tool and using
    // the selection modifiers.
    if (current_tool->getInk(0)->isSelection()) {
      if (m_customizationDelegate->isAddSelectionPressed() ||
          m_customizationDelegate->isSubtractSelectionPressed())
        return;
    }

    tools::Tool* old_quicktool = m_quicktool;
    tools::Tool* new_quicktool = m_customizationDelegate->getQuickTool(current_tool);

    // Check if the current state accept the given quicktool.
    if (new_quicktool && !m_state->acceptQuickTool(new_quicktool))
      return;

    // Hide the drawing cursor with the current tool brush size before
    // we change the quicktool. In this way we avoid using the
    // quicktool brush size to clean the current tool cursor.
    //
    // TODO Remove EditorState::regenerateDrawingCursor() creating a
    // new Document concept of multiple extra cels: we need an extra
    // cel for the drawing cursor, other for the moving pixels,
    // etc. In this way we'll not have conflicts between different
    // uses of the same extra cel.
    if (m_state->regenerateDrawingCursor())
      hideDrawingCursor();

    m_quicktool = new_quicktool;

    if (m_state->regenerateDrawingCursor())
      showDrawingCursor();

    // If the tool has changed, we must to update the status bar because
    // the new tool can display something different in the status bar (e.g. Eyedropper)
    if (old_quicktool != m_quicktool) {
      m_state->onQuickToolChange(this);

      updateStatusBar();

      App::instance()->getMainWindow()->getContextBar()
        ->updateFromTool(getCurrentEditorTool());
    }
  }
}

void Editor::updateContextBarFromModifiers()
{
  // We update the selection mode only if we're not selecting.
  if (hasCapture())
    return;

  ContextBar* ctxBar = App::instance()->getMainWindow()->getContextBar();

  // Selection mode

  SelectionMode mode = UIContext::instance()->settings()->selection()->getSelectionMode();

  if (m_customizationDelegate && m_customizationDelegate->isAddSelectionPressed())
    mode = kAddSelectionMode;
  else if (m_customizationDelegate && m_customizationDelegate->isSubtractSelectionPressed())
    mode = kSubtractSelectionMode;
  else if (m_secondaryButton)
    mode = kSubtractSelectionMode;

  if (mode != m_selectionMode) {
    m_selectionMode = mode;
    ctxBar->updateSelectionMode(mode);
  }

  // Move tool options

  bool autoSelectLayer = UIContext::instance()->settings()->getAutoSelectLayer();

  if (m_customizationDelegate && m_customizationDelegate->isAutoSelectLayerPressed())
    autoSelectLayer = true;

  if (m_autoSelectLayer != autoSelectLayer) {
    m_autoSelectLayer = autoSelectLayer;
    ctxBar->updateAutoSelectLayer(autoSelectLayer);
  }
}

//////////////////////////////////////////////////////////////////////
// Message handler for the editor

bool Editor::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

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
      updateQuicktool();
      updateContextBarFromModifiers();
      break;

    case kMouseLeaveMessage:
      hideDrawingCursor();
      StatusBar::instance()->clearText();
      break;

    case kMouseDownMessage:
      if (m_sprite) {
        MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);

        m_oldPos = mouseMsg->position();
        if (!m_secondaryButton && mouseMsg->right()) {
          m_secondaryButton = mouseMsg->right();

          updateQuicktool();
          updateContextBarFromModifiers();
          editor_setcursor();
        }

        EditorStatePtr holdState(m_state);
        return m_state->onMouseDown(this, mouseMsg);
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
        bool result = m_state->onMouseUp(this, static_cast<MouseMessage*>(msg));

        if (!hasCapture()) {
          m_secondaryButton = false;

          updateQuicktool();
          updateContextBarFromModifiers();
          editor_setcursor();
        }

        if (result)
          return true;
      }
      break;

    case kKeyDownMessage:
      if (m_sprite) {
        EditorStatePtr holdState(m_state);
        bool used = m_state->onKeyDown(this, static_cast<KeyMessage*>(msg));

        if (hasMouse()) {
          updateQuicktool();
          updateContextBarFromModifiers();
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
          updateQuicktool();
          updateContextBarFromModifiers();
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

    m_offset_x = std::max<int>(vp.w/2, vp.w - m_sprite->width()/2);
    m_offset_y = std::max<int>(vp.h/2, vp.h - m_sprite->height()/2);

    sz.w = (m_sprite->width() << m_zoom) + m_offset_x*2;
    sz.h = (m_sprite->height() << m_zoom) + m_offset_y*2;
  }
  else {
    sz.w = 4;
    sz.h = 4;
  }
  ev.setPreferredSize(sz);
}

void Editor::onPaint(ui::PaintEvent& ev)
{
  Graphics* g = ev.getGraphics();
  gfx::Rect rc = getClientBounds();
  SkinTheme* theme = static_cast<SkinTheme*>(this->getTheme());

  int old_cursor_thick = m_cursor_thick;
  if (m_cursor_thick)
    clearBrushPreview();

  // Editor without sprite
  if (!m_sprite) {
    g->fillRect(theme->getColor(ThemeColor::EditorFace), rc);
  }
  // Editor with sprite
  else {
    try {
      // Lock the sprite to read/render it.
      DocumentReader documentReader(m_document);

      // Draw the sprite in the editor
      drawSpriteUnclippedRect(g, gfx::Rect(0, 0, m_sprite->width(), m_sprite->height()));

      // Draw the mask boundaries
      if (m_document->getBoundariesSegments()) {
        drawMask(g);
        m_mask_timer.start();
      }
      else {
        m_mask_timer.stop();
      }

      // Draw the cursor again
      if (old_cursor_thick != 0) {
        drawBrushPreview(jmouse_x(0), jmouse_y(0));
      }
    }
    catch (const LockedDocumentException&) {
      // The sprite is locked to be read, so we can draw an opaque
      // background only.
      g->fillRect(theme->getColor(ThemeColor::EditorFace), rc);
    }
  }
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
    (m_selectionMode != kSubtractSelectionMode) &&
    m_document != NULL &&
    m_document->isMaskVisible() &&
    m_document->mask()->containsPoint(x, y);
}

void Editor::setZoomAndCenterInMouse(int zoom, int mouse_x, int mouse_y, ZoomBehavior zoomBehavior)
{
  View* view = View::getView(this);
  Rect vp = view->getViewportBounds();
  int x, y;
  bool centerMouse = false;
  int mx, my;

  switch (zoomBehavior) {
    case kCofiguredZoomBehavior:
      centerMouse = get_config_bool("Editor", "CenterOnZoom", false);
      break;
    case kCenterOnZoom:
      centerMouse = true;
      break;
    case kDontCenterOnZoom:
      centerMouse = false;
      break;
  }

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
    bool blit_valid_rgn = (m_zoom == zoom);

    m_zoom = zoom;

    updateEditor();
    setEditorScroll(x, y, blit_valid_rgn);
  }
  showDrawingCursor();
}

void Editor::pasteImage(const Image* image, int x, int y)
{
  // Change to a selection tool: it's necessary for PixelsMovement
  // which will use the extra cel for transformation preview, and is
  // not compatible with the drawing cursor preview which overwrite
  // the extra cel.
  if (!getCurrentEditorInk()->isSelection()) {
    tools::Tool* defaultSelectionTool =
      App::instance()->getToolBox()->getToolById(tools::WellKnownTools::RectangularMarquee);

    ToolBar::instance()->selectTool(defaultSelectionTool);
  }

  Document* document = this->document();
  int opacity = 255;
  Sprite* sprite = this->sprite();
  Layer* layer = this->layer();

  // Check bounds where the image will be pasted.
  {
    // Then we check if the image will be visible by the user.
    Rect visibleBounds = getVisibleSpriteBounds();
    x = MID(visibleBounds.x-image->width(), x, visibleBounds.x+visibleBounds.w-1);
    y = MID(visibleBounds.y-image->height(), y, visibleBounds.y+visibleBounds.h-1);

    // If the visible part of the pasted image will not fit in the
    // visible bounds of the editor, we put the image in the center of
    // the visible bounds.
    Rect visiblePasted = visibleBounds.createIntersect(gfx::Rect(x, y, image->width(), image->height()));
    if (((visibleBounds.w >= image->width() && visiblePasted.w < image->width()/2) ||
         (visibleBounds.w <  image->width() && visiblePasted.w < visibleBounds.w/2)) ||
        ((visibleBounds.h >= image->height() && visiblePasted.h < image->width()/2) ||
         (visibleBounds.h <  image->height() && visiblePasted.h < visibleBounds.h/2))) {
      x = visibleBounds.x + visibleBounds.w/2 - image->width()/2;
      y = visibleBounds.y + visibleBounds.h/2 - image->height()/2;
    }

    // We limit the image inside the sprite's bounds.
    x = MID(0, x, sprite->width() - image->width());
    y = MID(0, y, sprite->height() - image->height());
  }

  PixelsMovementPtr pixelsMovement(
    new PixelsMovement(UIContext::instance(),
      document, sprite, layer,
      image, x, y, opacity, "Paste"));

  // Select the pasted image so the user can move it and transform it.
  pixelsMovement->maskImage(image, x, y);

  setState(EditorStatePtr(new MovingPixelsState(this, NULL, pixelsMovement, NoHandle)));
}

void Editor::startSelectionTransformation(const gfx::Point& move)
{
  if (MovingPixelsState* movingPixels = dynamic_cast<MovingPixelsState*>(m_state.get())) {
    movingPixels->translate(move.x, move.y);
  }
  else if (StandbyState* standby = dynamic_cast<StandbyState*>(m_state.get())) {
    standby->startSelectionTransformation(this, move);
  }
}

void Editor::notifyScrollChanged()
{
  m_observers.notifyScrollChanged(this);
}

// static
ImageBufferPtr Editor::getRenderImageBuffer()
{
  return render_buffer;
}

void Editor::onSetTiledMode(filters::TiledMode mode)
{
  invalidate();
}

void Editor::onSetGridVisible(bool state)
{
  invalidate();
}

void Editor::onSetGridBounds(const gfx::Rect& rect)
{
  invalidate();
}

void Editor::onSetGridColor(const app::Color& color)
{
  invalidate();
}

} // namespace app
