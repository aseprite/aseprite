// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

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
#include "app/ini_file.h"
#include "app/modules/editors.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/pref/preferences.h"
#include "app/tools/ink.h"
#include "app/tools/tool.h"
#include "app/tools/tool_box.h"
#include "app/ui/color_bar.h"
#include "app/ui/context_bar.h"
#include "app/ui/editor/editor_customization_delegate.h"
#include "app/ui/editor/editor_decorator.h"
#include "app/ui/editor/moving_pixels_state.h"
#include "app/ui/editor/pixels_movement.h"
#include "app/ui/editor/play_state.h"
#include "app/ui/editor/standby_state.h"
#include "app/ui/main_window.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui/toolbar.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "base/convert_to.h"
#include "base/unique_ptr.h"
#include "doc/conversion_she.h"
#include "doc/doc.h"
#include "doc/document_event.h"
#include "doc/mask_boundaries.h"
#include "she/surface.h"
#include "she/system.h"
#include "ui/ui.h"

#include <cmath>
#include <cstdio>

namespace app {

using namespace app::skin;
using namespace gfx;
using namespace ui;
using namespace render;

class EditorPreRenderImpl : public EditorPreRender {
public:
  EditorPreRenderImpl(Editor* editor, Image* image, const Point& offset, Zoom zoom)
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
      m_offset.x + m_zoom.apply(rect.x),
      m_offset.y + m_zoom.apply(rect.y),
      m_offset.x + m_zoom.apply(rect.x+rect.w) - 1,
      m_offset.y + m_zoom.apply(rect.y+rect.h) - 1, rgbaColor, opacity);
  }

private:
  Editor* m_editor;
  Image* m_image;
  Point m_offset;
  Zoom m_zoom;
};

class EditorPostRenderImpl : public EditorPostRender {
public:
  EditorPostRenderImpl(Editor* editor, Graphics* g)
    : m_editor(editor)
    , m_g(g) {
  }

  Editor* getEditor() override {
    return m_editor;
  }

  void drawLine(int x1, int y1, int x2, int y2, gfx::Color screenColor) override {
    gfx::Point a(x1, y1);
    gfx::Point b(x2, y2);
    a = m_editor->editorToScreen(a);
    b = m_editor->editorToScreen(b);
    gfx::Rect bounds = m_editor->getBounds();
    a.x -= bounds.x;
    a.y -= bounds.y;
    b.x -= bounds.x;
    b.y -= bounds.y;
    m_g->drawLine(screenColor, a, b);
  }

  void drawRectXor(const gfx::Rect& rc) override {
    gfx::Rect rc2 = m_editor->editorToScreen(rc);
    gfx::Rect bounds = m_editor->getBounds();
    rc2.x -= bounds.x;
    rc2.y -= bounds.y;

    m_g->setDrawMode(Graphics::DrawMode::Xor);
    m_g->drawRect(gfx::rgba(255, 255, 255), rc2);
    m_g->setDrawMode(Graphics::DrawMode::Solid);
  }

private:
  Editor* m_editor;
  Graphics* m_g;
};

// static
doc::ImageBufferPtr Editor::m_renderBuffer;

// static
AppRender Editor::m_renderEngine;

Editor::Editor(Document* document, EditorFlags flags)
  : Widget(editor_type())
  , m_state(new StandbyState())
  , m_decorator(NULL)
  , m_document(document)
  , m_sprite(m_document->sprite())
  , m_layer(m_sprite->folder()->getFirstLayer())
  , m_frame(frame_t(0))
  , m_zoom(1, 1)
  , m_brushPreview(this)
  , m_lastDrawingPosition(-1, -1)
  , m_quicktool(NULL)
  , m_selectionMode(tools::SelectionMode::DEFAULT)
  , m_padding(0, 0)
  , m_antsTimer(100, this)
  , m_antsOffset(0)
  , m_customizationDelegate(NULL)
  , m_docView(NULL)
  , m_flags(flags)
  , m_secondaryButton(false)
  , m_aniSpeed(1.0)
{
  // Add the first state into the history.
  m_statesHistory.push(m_state);

  this->setFocusStop(true);

  m_currentToolChangeConn =
    Preferences::instance().toolBox.activeTool.AfterChange.connect(
      Bind<void>(&Editor::onCurrentToolChange, this));

  m_fgColorChangeConn =
    Preferences::instance().colorBar.fgColor.AfterChange.connect(
      Bind<void>(&Editor::onFgColorChange, this));

  m_contextBarBrushChangeConn =
    App::instance()->getMainWindow()->getContextBar()->BrushChange.connect(
      Bind<void>(&Editor::onContextBarBrushChange, this));

  DocumentPreferences& docPref = Preferences::instance().document(m_document);

  // Restore last site in preferences
  frame_t preferredFrame = docPref.site.frame();
  Layer* preferredLayer = m_sprite->indexToLayer(docPref.site.layer());
  if (preferredFrame >= 0 && preferredFrame <= m_sprite->lastFrame())
    setFrame(preferredFrame);
  if (preferredLayer)
    setLayer(preferredLayer);

  m_tiledConn = docPref.tiled.AfterChange.connect(Bind<void>(&Editor::invalidate, this));
  m_gridConn = docPref.grid.AfterChange.connect(Bind<void>(&Editor::invalidate, this));
  m_pixelGridConn = docPref.pixelGrid.AfterChange.connect(Bind<void>(&Editor::invalidate, this));
  m_onionskinConn = docPref.onionskin.AfterChange.connect(Bind<void>(&Editor::invalidate, this));

  m_document->addObserver(this);

  m_state->onEnterState(this);
}

Editor::~Editor()
{
  if (m_document && m_sprite) {
    DocumentPreferences& docPref = Preferences::instance()
      .document(m_document);
    docPref.site.frame(frame());
    docPref.site.layer(m_sprite->layerToIndex(layer()));
  }

  m_observers.notifyDestroyEditor(this);
  m_document->removeObserver(this);

  setCustomizationDelegate(NULL);

  m_antsTimer.stop();
}

void Editor::destroyEditorSharedInternals()
{
  m_renderBuffer.reset();
}

bool Editor::isActive() const
{
  return (current_editor == this);
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
  m_brushPreview.hide();

  // Fire before change state event, set the state, and fire after
  // change state event.
  EditorState::LeaveAction leaveAction =
    m_state->onLeaveState(this, newState.get());

  // Push a new state
  if (newState) {
    if (leaveAction == EditorState::DiscardState)
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

  ASSERT(m_state);

  // Change to the new state.
  m_state->onEnterState(this);

  // Notify observers
  m_observers.notifyStateChanged(this);

  // Setup the new mouse cursor
  setCursor(ui::get_mouse_position());

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
  bool changed = (m_layer != layer);

  m_observers.notifyBeforeLayerChanged(this);
  m_layer = const_cast<Layer*>(layer);
  m_observers.notifyAfterLayerChanged(this);

  // If the onion skinning depends on the active layer, we've to
  // redraw the whole editor.
  if (m_document && changed) {
    DocumentPreferences& docPref = Preferences::instance().document(m_document);
    if (docPref.onionskin.currentLayer())
      invalidate();
  }

  // The active layer has changed.
  if (isActive())
    UIContext::instance()->notifyActiveSiteChanged();

  updateStatusBar();
}

void Editor::setFrame(frame_t frame)
{
  if (m_frame == frame)
    return;

  m_observers.notifyBeforeFrameChanged(this);
  {
    HideBrushPreview hide(m_brushPreview);
    m_frame = frame;
  }
  m_observers.notifyAfterFrameChanged(this);

  // The active frame has changed.
  if (isActive())
    UIContext::instance()->notifyActiveSiteChanged();

  invalidate();
  updateStatusBar();
}

void Editor::getSite(Site* site) const
{
  site->document(m_document);
  site->sprite(m_sprite);
  site->layer(m_layer);
  site->frame(m_frame);
}

Site Editor::getSite() const
{
  Site site;
  getSite(&site);
  return site;
}

void Editor::setDefaultScroll()
{
  View* view = View::getView(this);
  Rect vp = view->getViewportBounds();

  setEditorScroll(
    gfx::Point(
      m_padding.x - vp.w/2 + m_zoom.apply(m_sprite->width())/2,
      m_padding.y - vp.h/2 + m_zoom.apply(m_sprite->height())/2), false);
}

// Sets the scroll position of the editor
void Editor::setEditorScroll(const gfx::Point& scroll, bool blitValidRegion)
{
  HideBrushPreview hide(m_brushPreview);
  View* view = View::getView(this);
  Point oldScroll;
  Region region;

  if (blitValidRegion) {
    getDrawableRegion(region, kCutTopWindows);
    oldScroll = view->getViewScroll();
  }

  view->setViewScroll(scroll);
  Point newScroll = view->getViewScroll();

  if (blitValidRegion) {
    // Move screen with blits
    scrollRegion(region, oldScroll - newScroll);
  }
}

void Editor::setEditorZoom(const render::Zoom& zoom)
{
  setZoomAndCenterInMouse(
    zoom, ui::get_mouse_position(),
    Editor::ZoomBehavior::CENTER);
}

void Editor::updateEditor()
{
  View::getView(this)->updateView();
}

void Editor::drawOneSpriteUnclippedRect(ui::Graphics* g, const gfx::Rect& spriteRectToDraw, int dx, int dy)
{
  // Clip from sprite and apply zoom
  gfx::Rect rc = m_sprite->bounds().createIntersection(spriteRectToDraw);
  rc = m_zoom.apply(rc);

  int dest_x = dx + m_padding.x + rc.x;
  int dest_y = dy + m_padding.y + rc.y;

  // Clip from graphics/screen
  const gfx::Rect& clip = g->getClipBounds();
  if (dest_x < clip.x) {
    rc.x += clip.x - dest_x;
    rc.w -= clip.x - dest_x;
    dest_x = clip.x;
  }
  if (dest_y < clip.y) {
    rc.y += clip.y - dest_y;
    rc.h -= clip.y - dest_y;
    dest_y = clip.y;
  }
  if (dest_x+rc.w > clip.x+clip.w) {
    rc.w = clip.x+clip.w-dest_x;
  }
  if (dest_y+rc.h > clip.y+clip.h) {
    rc.h = clip.y+clip.h-dest_y;
  }

  if (rc.isEmpty())
    return;

  // Generate the rendered image
  if (!m_renderBuffer)
    m_renderBuffer.reset(new doc::ImageBuffer());

  base::UniquePtr<Image> rendered(NULL);
  try {
    // Generate a "expose sprite pixels" notification. This is used by
    // tool managers that need to validate this region (copy pixels from
    // the original cel) before it can be used by the RenderEngine.
    {
      gfx::Rect expose = m_zoom.remove(rc);
      // If the zoom level is less than 100%, we add extra pixels to
      // the exposed area. Those pixels could be shown in the
      // rendering process depending on each cel position.
      // E.g. when we are drawing in a cel with position < (0,0)
      if (m_zoom.scale() < 1.0) {
        expose.enlarge(int(1./m_zoom.scale()));
      }
      // If the zoom level is more than %100 we add an extra pixel to
      // expose just in case the zoom requires to display it.  Note:
      // this is really necessary to avoid showing invalid destination
      // areas in ToolLoopImpl.
      else if (m_zoom.scale() > 1.0) {
        expose.enlarge(1);
      }
      m_document->notifyExposeSpritePixels(m_sprite, gfx::Region(expose));
    }

    // Create a temporary RGB bitmap to draw all to it
    rendered.reset(Image::create(IMAGE_RGB, rc.w, rc.h, m_renderBuffer));
    m_renderEngine.setupBackground(m_document, rendered->pixelFormat());
    m_renderEngine.disableOnionskin();

    if ((m_flags & kShowOnionskin) == kShowOnionskin) {
      DocumentPreferences& docPref = Preferences::instance()
        .document(m_document);

      if (docPref.onionskin.active()) {
        OnionskinOptions opts(
          (docPref.onionskin.type() == app::gen::OnionskinType::MERGE ?
           render::OnionskinType::MERGE:
           (docPref.onionskin.type() == app::gen::OnionskinType::RED_BLUE_TINT ?
            render::OnionskinType::RED_BLUE_TINT:
            render::OnionskinType::NONE)));

        opts.position(docPref.onionskin.position());
        opts.prevFrames(docPref.onionskin.prevFrames());
        opts.nextFrames(docPref.onionskin.nextFrames());
        opts.opacityBase(docPref.onionskin.opacityBase());
        opts.opacityStep(docPref.onionskin.opacityStep());
        opts.layer(docPref.onionskin.currentLayer() ? m_layer: nullptr);

        FrameTag* tag = nullptr;
        if (docPref.onionskin.loopTag())
          tag = m_sprite->frameTags().innerTag(m_frame);
        opts.loopTag(tag);

        m_renderEngine.setOnionskin(opts);
      }
    }

    ExtraCelRef extraCel = m_document->extraCel();
    if (extraCel && extraCel->type() != render::ExtraType::NONE) {
      m_renderEngine.setExtraImage(
        extraCel->type(),
        extraCel->cel(),
        extraCel->image(),
        extraCel->blendMode(),
        m_layer, m_frame);
    }

    m_renderEngine.renderSprite(rendered, m_sprite, m_frame,
      gfx::Clip(0, 0, rc), m_zoom);

    m_renderEngine.removeExtraImage();
  }
  catch (const std::exception& e) {
    Console::showException(e);
  }

  if (rendered) {
    // Pre-render decorator.
    if ((m_flags & kShowDecorators) && m_decorator) {
      EditorPreRenderImpl preRender(this, rendered,
        Point(-rc.x, -rc.y), m_zoom);
      m_decorator->preRenderDecorator(&preRender);
    }

    // Convert the render to a she::Surface
    static she::Surface* tmp;
    if (!tmp || tmp->width() < rc.w || tmp->height() < rc.h) {
      if (tmp)
        tmp->dispose();

      tmp = she::instance()->createRgbaSurface(rc.w, rc.h);
    }

    if (tmp->nativeHandle()) {
      convert_image_to_surface(rendered, m_sprite->palette(m_frame),
        tmp, 0, 0, 0, 0, rc.w, rc.h);

      g->blit(tmp, 0, 0, dest_x, dest_y, rc.w, rc.h);

      m_brushPreview.invalidateRegion(
        gfx::Region(
          gfx::Rect(dest_x, dest_y, rc.w, rc.h)));
    }
  }
}

void Editor::drawSpriteUnclippedRect(ui::Graphics* g, const gfx::Rect& _rc)
{
  gfx::Rect rc = _rc;
  // For odd zoom scales minor than 100% we have to add an extra window
  // just to make sure the whole rectangle is drawn.
  if (m_zoom.scale() < 1.0)
    rc.inflate(int(1./m_zoom.scale()), int(1./m_zoom.scale()));

  gfx::Rect client = getClientBounds();
  gfx::Rect spriteRect(
    client.x + m_padding.x,
    client.y + m_padding.y,
    m_zoom.apply(m_sprite->width()),
    m_zoom.apply(m_sprite->height()));
  gfx::Rect enclosingRect = spriteRect;

  // Draw the main sprite at the center.
  drawOneSpriteUnclippedRect(g, rc, 0, 0);

  gfx::Region outside(client);
  outside.createSubtraction(outside, gfx::Region(spriteRect));

  // Document preferences
  DocumentPreferences& docPref =
      Preferences::instance().document(m_document);

  if (int(docPref.tiled.mode()) & int(filters::TiledMode::X_AXIS)) {
    drawOneSpriteUnclippedRect(g, rc, -spriteRect.w, 0);
    drawOneSpriteUnclippedRect(g, rc, +spriteRect.w, 0);

    enclosingRect = gfx::Rect(spriteRect.x-spriteRect.w, spriteRect.y, spriteRect.w*3, spriteRect.h);
    outside.createSubtraction(outside, gfx::Region(enclosingRect));
  }

  if (int(docPref.tiled.mode()) & int(filters::TiledMode::Y_AXIS)) {
    drawOneSpriteUnclippedRect(g, rc, 0, -spriteRect.h);
    drawOneSpriteUnclippedRect(g, rc, 0, +spriteRect.h);

    enclosingRect = gfx::Rect(spriteRect.x, spriteRect.y-spriteRect.h, spriteRect.w, spriteRect.h*3);
    outside.createSubtraction(outside, gfx::Region(enclosingRect));
  }

  if (docPref.tiled.mode() == filters::TiledMode::BOTH) {
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
    g->fillRegion(theme->colors.editorFace(), outside);
  }

  // Grids
  {
    // Clipping
    gfx::Rect cliprc = editorToScreen(rc).offset(-getBounds().getOrigin());
    cliprc = cliprc.createIntersection(spriteRect);
    if (!cliprc.isEmpty()) {
      IntersectClip clip(g, cliprc);

      // Draw the pixel grid
      if ((m_zoom.scale() > 2.0) && docPref.pixelGrid.visible()) {
        int alpha = docPref.pixelGrid.opacity();

        if (docPref.pixelGrid.autoOpacity()) {
          alpha = int(alpha * (m_zoom.scale()-2.) / (16.-2.));
          alpha = MID(0, alpha, 255);
        }

        drawGrid(g, enclosingRect, Rect(0, 0, 1, 1),
          docPref.pixelGrid.color(), alpha);
      }

      // Draw the grid
      if (docPref.grid.visible()) {
        gfx::Rect gridrc = docPref.grid.bounds();
        if (m_zoom.apply(gridrc.w) > 2 &&
          m_zoom.apply(gridrc.h) > 2) {
          int alpha = docPref.grid.opacity();

          if (docPref.grid.autoOpacity()) {
            double len = (m_zoom.apply(gridrc.w) + m_zoom.apply(gridrc.h)) / 2.;
            alpha = int(alpha * len / 32.);
            alpha = MID(0, alpha, 255);
          }

          if (alpha > 8)
            drawGrid(g, enclosingRect, docPref.grid.bounds(),
              docPref.grid.color(), alpha);
        }
      }
    }
  }

  // Symmetry mode
  {
    switch (docPref.symmetry.mode()) {
      case app::gen::SymmetryMode::NONE:
        // Do nothing
        break;
      case app::gen::SymmetryMode::HORIZONTAL: {
        int x = docPref.symmetry.xAxis();
        if (x > 0) {
          gfx::Color color = color_utils::color_for_ui(docPref.grid.color());
          g->drawVLine(color,
                       enclosingRect.x + m_zoom.apply(x),
                       enclosingRect.y,
                       enclosingRect.h);
        }
        break;
      }
      case app::gen::SymmetryMode::VERTICAL: {
        int y = docPref.symmetry.yAxis();
        if (y > 0) {
          gfx::Color color = color_utils::color_for_ui(docPref.grid.color());
          g->drawHLine(color,
                       enclosingRect.x,
                       enclosingRect.y + m_zoom.apply(y),
                       enclosingRect.w);
        }
        break;
      }
    }
  }

  if (m_flags & kShowOutside) {
    // Draw the borders that enclose the sprite.
    enclosingRect.enlarge(1);
    g->drawRect(theme->colors.editorSpriteBorder(), enclosingRect);
    g->drawHLine(
      theme->colors.editorSpriteBottomBorder(),
      enclosingRect.x, enclosingRect.y+enclosingRect.h, enclosingRect.w);
  }

  // Draw the mask
  if (m_document->getMaskBoundaries())
    drawMask(g);

  // Post-render decorator.
  if ((m_flags & kShowDecorators) && m_decorator) {
    EditorPostRenderImpl postRender(this, g);
    m_decorator->postRenderDecorator(&postRender);
  }
}

void Editor::drawSpriteClipped(const gfx::Region& updateRegion)
{
  Region screenRegion;
  getDrawableRegion(screenRegion, kCutTopWindows);

  ScreenGraphics screenGraphics;
  GraphicsPtr editorGraphics = getGraphics(getClientBounds());

  for (const Rect& updateRect : updateRegion) {
    for (const Rect& screenRect : screenRegion) {
      IntersectClip clip(&screenGraphics, screenRect);
      if (clip)
        drawSpriteUnclippedRect(editorGraphics.get(), updateRect);
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

  ASSERT(m_document->getMaskBoundaries());

  int x = m_padding.x;
  int y = m_padding.y;

  for (const auto& seg : *m_document->getMaskBoundaries()) {
    CheckedDrawMode checked(g, m_antsOffset);
    gfx::Rect bounds = m_zoom.apply(seg.bounds());

    if (!seg.open()) {
      if (seg.vertical()) --bounds.x;
      else --bounds.y;
    }

    // The color doesn't matter, we are using CheckedDrawMode
    if (seg.vertical())
      g->drawVLine(gfx::rgba(0, 0, 0), x+bounds.x, y+bounds.y, bounds.h);
    else
      g->drawHLine(gfx::rgba(0, 0, 0), x+bounds.x, y+bounds.y, bounds.w);
  }
}

void Editor::drawMaskSafe()
{
  if ((m_flags & kShowMask) == 0)
    return;

  if (isVisible() &&
      m_document &&
      m_document->getMaskBoundaries()) {
    Region region;
    getDrawableRegion(region, kCutTopWindows);
    region.offset(-getBounds().getOrigin());

    HideBrushPreview hide(m_brushPreview);
    GraphicsPtr g = getGraphics(getClientBounds());

    for (const gfx::Rect& rc : region) {
      IntersectClip clip(g.get(), rc);
      if (clip)
        drawMask(g.get());
    }
  }
}

void Editor::drawGrid(Graphics* g, const gfx::Rect& spriteBounds, const Rect& gridBounds, const app::Color& color, int alpha)
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
  grid = editorToScreen(grid);
  if (grid.w < 1 || grid.h < 1)
    return;

  // Adjust for client area
  gfx::Rect bounds = getBounds();
  grid.offset(-bounds.getOrigin());

  while (grid.x-grid.w >= spriteBounds.x) grid.x -= grid.w;
  while (grid.y-grid.h >= spriteBounds.y) grid.y -= grid.h;

  // Get the grid's color
  gfx::Color grid_color = color_utils::color_for_ui(color);
  grid_color = gfx::rgba(
    gfx::getr(grid_color),
    gfx::getg(grid_color),
    gfx::getb(grid_color), alpha);

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
  if (!Preferences::instance().experimental.flashLayer())
    return;

  Site site = getSite();

  int x, y;
  const Image* src_image = site.image(&x, &y);
  if (src_image) {
    m_renderEngine.removePreviewImage();

    ExtraCelRef extraCel(new ExtraCel);
    extraCel->create(m_sprite, m_sprite->bounds(), m_frame, 255);
    extraCel->setType(render::ExtraType::COMPOSITE);
    extraCel->setBlendMode(BlendMode::NEG_BW);

    Image* flash_image = extraCel->image();
    clear_image(flash_image, flash_image->maskColor());
    copy_image(flash_image, src_image, x, y);

    {
      ExtraCelRef oldExtraCel = m_document->extraCel();
      m_document->setExtraCel(extraCel);
      drawSpriteClipped(gfx::Region(
                          gfx::Rect(0, 0, m_sprite->width(), m_sprite->height())));
      getManager()->flipDisplay();
      m_document->setExtraCel(oldExtraCel);
    }

    invalidate();
  }
}

gfx::Point Editor::autoScroll(MouseMessage* msg, AutoScroll dir, bool blitValidRegion)
{
  // // Hide the brush preview
  // HideBrushPreview hide(editor->brushPreview());
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
    setEditorScroll(scroll, blitValidRegion);

#if defined(_WIN32) || defined(__APPLE__)
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
  tools::Tool* tool = App::instance()->activeTool();
  return
    (tool->getInk(0)->isPaint() || tool->getInk(0)->isEffect()) &&
    (!tool->getInk(0)->isEraser());
}

tools::Tool* Editor::getCurrentEditorTool()
{
  if (m_quicktool)
    return m_quicktool;

  tools::Tool* tool = App::instance()->activeTool();

  if (m_secondaryButton && isCurrentToolAffectedByRightClickMode()) {
    tools::ToolBox* toolbox = App::instance()->getToolBox();

    switch (Preferences::instance().editor.rightClickMode()) {
      case app::gen::RightClickMode::PAINT_BGCOLOR:
        // Do nothing, use the current tool
        break;
      case app::gen::RightClickMode::PICK_FGCOLOR:
        tool = toolbox->getToolById(tools::WellKnownTools::Eyedropper);
        break;
      case app::gen::RightClickMode::ERASE:
        tool = toolbox->getToolById(tools::WellKnownTools::Eraser);
        break;
      case app::gen::RightClickMode::SCROLL:
        tool = toolbox->getToolById(tools::WellKnownTools::Hand);
        break;
    }
  }

  return tool;
}

tools::Ink* Editor::getCurrentEditorInk()
{
  tools::Ink* ink = m_state->getStateInk();
  if (ink)
    return ink;

  tools::Tool* tool = getCurrentEditorTool();
  ink = tool->getInk(m_secondaryButton ? 1: 0);

  if (m_quicktool)
    return ink;

  app::gen::RightClickMode rightClickMode = Preferences::instance().editor.rightClickMode();

  if (m_secondaryButton &&
      rightClickMode != app::gen::RightClickMode::DEFAULT &&
      isCurrentToolAffectedByRightClickMode()) {
    tools::ToolBox* toolbox = App::instance()->getToolBox();

    switch (rightClickMode) {
      case app::gen::RightClickMode::DEFAULT:
        // Do nothing
        break;
      case app::gen::RightClickMode::PICK_FGCOLOR:
        ink = toolbox->getInkById(tools::WellKnownInks::PickFg);
        break;
      case app::gen::RightClickMode::ERASE:
        ink = toolbox->getInkById(tools::WellKnownInks::Eraser);
        break;
      case app::gen::RightClickMode::SCROLL:
        ink = toolbox->getInkById(tools::WellKnownInks::Scroll);
        break;
    }
  }
  // Only paint tools can have different inks
  else if (ink->isPaint() && !ink->isEffect()) {
    tools::InkType inkType = Preferences::instance().tool(tool).ink();
    const char* id = NULL;

    switch (inkType) {

      case tools::InkType::SIMPLE: {
        id = tools::WellKnownInks::Paint;

        ColorBar* colorbar = ColorBar::instance();
        app::Color color = (m_secondaryButton ? colorbar->getBgColor():
                                                colorbar->getFgColor());
        if (color.getAlpha() == 0)
          id = tools::WellKnownInks::PaintCopy;
        break;
      }

      case tools::InkType::ALPHA_COMPOSITING:
        id = tools::WellKnownInks::Paint;
        break;
      case tools::InkType::COPY_COLOR:
        id = tools::WellKnownInks::PaintCopy;
        break;
      case tools::InkType::LOCK_ALPHA:
        id = tools::WellKnownInks::PaintLockAlpha;
        break;
      case tools::InkType::SHADING:
        id = tools::WellKnownInks::Shading;
        break;
    }

    if (id)
      ink = App::instance()->getToolBox()->getInkById(id);
  }

  return ink;
}

gfx::Point Editor::screenToEditor(const gfx::Point& pt)
{
  View* view = View::getView(this);
  Rect vp = view->getViewportBounds();
  Point scroll = view->getViewScroll();

  return gfx::Point(
    m_zoom.remove(pt.x - vp.x + scroll.x - m_padding.x),
    m_zoom.remove(pt.y - vp.y + scroll.y - m_padding.y));
}

Point Editor::editorToScreen(const gfx::Point& pt)
{
  View* view = View::getView(this);
  Rect vp = view->getViewportBounds();
  Point scroll = view->getViewScroll();

  return Point(
    (vp.x - scroll.x + m_padding.x + m_zoom.apply(pt.x)),
    (vp.y - scroll.y + m_padding.y + m_zoom.apply(pt.y)));
}

Rect Editor::screenToEditor(const Rect& rc)
{
  return gfx::Rect(
    screenToEditor(rc.getOrigin()),
    screenToEditor(rc.getPoint2()));
}

Rect Editor::editorToScreen(const Rect& rc)
{
  return gfx::Rect(
    editorToScreen(rc.getOrigin()),
    editorToScreen(rc.getPoint2()));
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
  vp = screenToEditor(vp);

  return vp.createIntersection(m_sprite->bounds());
}

// Changes the scroll to see the given point as the center of the editor.
void Editor::centerInSpritePoint(const gfx::Point& spritePos)
{
  HideBrushPreview hide(m_brushPreview);
  View* view = View::getView(this);
  Rect vp = view->getViewportBounds();

  gfx::Point scroll(
    m_padding.x - (vp.w/2) + m_zoom.apply(1)/2 + m_zoom.apply(spritePos.x),
    m_padding.y - (vp.h/2) + m_zoom.apply(1)/2 + m_zoom.apply(spritePos.y));

  updateEditor();
  setEditorScroll(scroll, false);
  invalidate();
}

void Editor::updateStatusBar()
{
  if (!hasMouse())
    return;

  // Setup status bar using the current editor's state
  m_state->onUpdateStatusBar(this);
}

void Editor::updateQuicktool()
{
  if (m_customizationDelegate) {
    tools::Tool* current_tool = App::instance()->activeTool();

    // Don't change quicktools if we are in a selection tool and using
    // the selection modifiers.
    if (current_tool->getInk(0)->isSelection() &&
        int(m_customizationDelegate->getPressedKeyAction(KeyContext::SelectionTool)) != 0)
      return;

    tools::Tool* old_quicktool = m_quicktool;
    tools::Tool* new_quicktool = m_customizationDelegate->getQuickTool(current_tool);

    // Check if the current state accept the given quicktool.
    if (new_quicktool &&
        !m_state->acceptQuickTool(new_quicktool))
      return;

    // If the quick tool has changed we update the brush cursor, the
    // context bar, and the status bar. Things that depends on the
    // current tool.
    //
    // TODO We could add a quick tool observer for this
    if (old_quicktool != new_quicktool) {
      // Hide the brush preview with the current tool brush size
      // before we change the quicktool. In this way we avoid using
      // the quicktool brush size to clean the current tool cursor.
      //
      // TODO Create a new Document concept of multiple extra cels: we
      // need an extra cel for the drawing cursor, other for the moving
      // pixels, etc. In this way we'll not have conflicts between
      // different uses of the same extra cel.
      {
        HideBrushPreview hide(m_brushPreview);
        m_quicktool = new_quicktool;
      }

      m_state->onQuickToolChange(this);

      updateStatusBar();

      App::instance()->getMainWindow()->getContextBar()
        ->updateForTool(getCurrentEditorTool());
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

  tools::SelectionMode mode = Preferences::instance().selection.mode();

  KeyAction action = KeyAction::None;
  if (m_customizationDelegate)
    action = m_customizationDelegate->getPressedKeyAction(KeyContext::SelectionTool);

  if (int(action & KeyAction::AddSelection))
    mode = tools::SelectionMode::ADD;
  if (int(action & KeyAction::SubtractSelection))
    mode = tools::SelectionMode::SUBTRACT;
  else if (m_secondaryButton)
    mode = tools::SelectionMode::SUBTRACT;

  if (mode != m_selectionMode) {
    m_selectionMode = mode;
    ctxBar->updateSelectionMode(mode);
  }

  // Move tool options

  bool autoSelectLayer = Preferences::instance().editor.autoSelectLayer();

  if ((m_customizationDelegate) &&
      int(m_customizationDelegate->getPressedKeyAction(KeyContext::MoveTool) & KeyAction::AutoSelectLayer))
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
      if (static_cast<TimerMessage*>(msg)->timer() == &m_antsTimer) {
        if (isVisible() && m_sprite) {
          drawMaskSafe();

          // Set offset to make selection-movement effect
          if (m_antsOffset < 7)
            m_antsOffset++;
          else
            m_antsOffset = 0;
        }
        else if (m_antsTimer.isRunning()) {
          m_antsTimer.stop();
        }
      }
      break;

    case kMouseEnterMessage:
      updateQuicktool();
      updateContextBarFromModifiers();
      break;

    case kMouseLeaveMessage:
      m_brushPreview.hide();
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
          setCursor(mouseMsg->position());
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
        MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
        bool result = m_state->onMouseUp(this, mouseMsg);

        if (!hasCapture()) {
          m_secondaryButton = false;

          updateQuicktool();
          updateContextBarFromModifiers();
          setCursor(mouseMsg->position());
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
          setCursor(ui::get_mouse_position());
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
          setCursor(ui::get_mouse_position());
        }

        if (used)
          return true;
      }
      break;

    case kFocusLeaveMessage:
      // As we use keys like Space-bar as modifier, we can clear the
      // keyboard buffer when we lost the focus.
      she::clear_keyboard_buffer();
      break;

    case kMouseWheelMessage:
      if (m_sprite && hasMouse()) {
        EditorStatePtr holdState(m_state);
        if (m_state->onMouseWheel(this, static_cast<MouseMessage*>(msg)))
          return true;
      }
      break;

    case kSetCursorMessage:
      setCursor(static_cast<MouseMessage*>(msg)->position());
      return true;
  }

  return Widget::onProcessMessage(msg);
}

void Editor::onPreferredSize(PreferredSizeEvent& ev)
{
  gfx::Size sz(0, 0);

  if (m_sprite) {
    gfx::Point padding = calcExtraPadding(m_zoom);
    sz.w = m_zoom.apply(m_sprite->width()) + padding.x*2;
    sz.h = m_zoom.apply(m_sprite->height()) + padding.y*2;
  }
  else {
    sz.w = 4;
    sz.h = 4;
  }
  ev.setPreferredSize(sz);
}

void Editor::onResize(ui::ResizeEvent& ev)
{
  Widget::onResize(ev);
  m_padding = calcExtraPadding(m_zoom);
}

void Editor::onPaint(ui::PaintEvent& ev)
{
  HideBrushPreview hide(m_brushPreview);
  Graphics* g = ev.getGraphics();
  gfx::Rect rc = getClientBounds();
  SkinTheme* theme = static_cast<SkinTheme*>(this->getTheme());

  // Editor without sprite
  if (!m_sprite) {
    g->fillRect(theme->colors.editorFace(), rc);
  }
  // Editor with sprite
  else {
    try {
      // Lock the sprite to read/render it.
      DocumentReader documentReader(m_document, 0);

      // Draw the sprite in the editor
      drawSpriteUnclippedRect(g, gfx::Rect(0, 0, m_sprite->width(), m_sprite->height()));

      // Draw the mask boundaries
      if (m_document->getMaskBoundaries()) {
        drawMask(g);
        m_antsTimer.start();
      }
      else {
        m_antsTimer.stop();
      }
    }
    catch (const LockedDocumentException&) {
      // The sprite is locked to be read, so we can draw an opaque
      // background only.
      g->fillRect(theme->colors.editorFace(), rc);
      defer_invalid_rect(g->getClipBounds().offset(getBounds().getOrigin()));
    }
  }
}

void Editor::onInvalidateRegion(const gfx::Region& region)
{
  Widget::onInvalidateRegion(region);
  m_brushPreview.invalidateRegion(region);
}

// When the current tool is changed
void Editor::onCurrentToolChange()
{
  m_state->onCurrentToolChange(this);
}

void Editor::onFgColorChange()
{
  m_brushPreview.redraw();
}

void Editor::onContextBarBrushChange()
{
  m_brushPreview.redraw();
}

void Editor::onExposeSpritePixels(doc::DocumentEvent& ev)
{
  if (m_state && ev.sprite() == m_sprite)
    m_state->onExposeSpritePixels(ev.region());
}

void Editor::setCursor(const gfx::Point& mouseScreenPos)
{
  bool used = false;
  if (m_sprite)
    used = m_state->onSetCursor(this, mouseScreenPos);

  if (!used)
    showMouseCursor(kArrowCursor);
}

void Editor::setLastDrawingPosition(const gfx::Point& pos)
{
  m_lastDrawingPosition = pos;
}

bool Editor::canDraw()
{
  return (m_layer != NULL &&
          m_layer->isImage() &&
          m_layer->isVisible() &&
          m_layer->isEditable());
}

bool Editor::isInsideSelection()
{
  gfx::Point spritePos = screenToEditor(ui::get_mouse_position());
  return
    (m_selectionMode != tools::SelectionMode::SUBTRACT) &&
     m_document != NULL &&
     m_document->isMaskVisible() &&
     m_document->mask()->containsPoint(spritePos.x, spritePos.y);
}

void Editor::setZoomAndCenterInMouse(const Zoom& zoom,
  const gfx::Point& mousePos, ZoomBehavior zoomBehavior)
{
  HideBrushPreview hide(m_brushPreview);
  View* view = View::getView(this);
  Rect vp = view->getViewportBounds();

  gfx::Point screenPos;
  gfx::Point spritePos;
  gfx::PointT<double> subpixelPos(0.5, 0.5);

  switch (zoomBehavior) {
    case ZoomBehavior::CENTER:
      screenPos = gfx::Point(vp.x + vp.w/2,
                             vp.y + vp.h/2);
      break;
    case ZoomBehavior::MOUSE:
      screenPos = mousePos;
      break;
  }
  spritePos = screenToEditor(screenPos);

  if (zoomBehavior == ZoomBehavior::MOUSE &&
      m_zoom.scale() > 1.0) {
    gfx::Point screenPos2 = editorToScreen(spritePos);
    subpixelPos.x = (0.5 + screenPos.x - screenPos2.x) / m_zoom.scale();
    subpixelPos.y = (0.5 + screenPos.y - screenPos2.y) / m_zoom.scale();

    if (zoom.scale() > m_zoom.scale()) {
      double t = 1.0 / zoom.scale();
      if (subpixelPos.x >= 0.5-t && subpixelPos.x <= 0.5+t) subpixelPos.x = 0.5;
      if (subpixelPos.y >= 0.5-t && subpixelPos.y <= 0.5+t) subpixelPos.y = 0.5;
    }
  }

  gfx::Point padding = calcExtraPadding(zoom);
  gfx::Point scrollPos(
    padding.x - (screenPos.x-vp.x) + zoom.apply(spritePos.x+zoom.remove(1)/2) + int(zoom.apply(subpixelPos.x)),
    padding.y - (screenPos.y-vp.y) + zoom.apply(spritePos.y+zoom.remove(1)/2) + int(zoom.apply(subpixelPos.y)));

  if ((m_zoom != zoom) || (screenPos != view->getViewScroll())) {
    bool blitValidRegion = (m_zoom == zoom);

    m_zoom = zoom;

    updateEditor();
    setEditorScroll(scrollPos, blitValidRegion);
  }

  flushRedraw();
}

void Editor::pasteImage(const Image* image, const Mask* mask)
{
  ASSERT(image);
  ASSERT(mask);

  // Change to a selection tool: it's necessary for PixelsMovement
  // which will use the extra cel for transformation preview, and is
  // not compatible with the drawing cursor preview which overwrite
  // the extra cel.
  if (!getCurrentEditorInk()->isSelection()) {
    tools::Tool* defaultSelectionTool =
      App::instance()->getToolBox()->getToolById(tools::WellKnownTools::RectangularMarquee);

    ToolBar::instance()->selectTool(defaultSelectionTool);
  }

  Sprite* sprite = this->sprite();

  // Check bounds where the image will be pasted.
  int x = mask->bounds().x;
  int y = mask->bounds().y;
  {
    Rect visibleBounds = getVisibleSpriteBounds();

    // If the pasted image original location center point isn't
    // visible, we center the image in the editor's visible bounds.
    if (!visibleBounds.contains(mask->bounds().getCenter())) {
      x = visibleBounds.x + visibleBounds.w/2 - image->width()/2;
      y = visibleBounds.y + visibleBounds.h/2 - image->height()/2;
    }
    // In other case, if the center is visible, we put the pasted
    // image in its original location.
    else {
      x = MID(visibleBounds.x-image->width(), x, visibleBounds.x+visibleBounds.w-1);
      y = MID(visibleBounds.y-image->height(), y, visibleBounds.y+visibleBounds.h-1);
    }

    // Also we always limit the image inside the sprite's bounds.
    x = MID(0, x, sprite->width() - image->width());
    y = MID(0, y, sprite->height() - image->height());
  }

  // Clear brush preview, as the extra cel will be replaced with the
  // pasted image.
  m_brushPreview.hide();

  Mask mask2(*mask);
  mask2.setOrigin(x, y);

  PixelsMovementPtr pixelsMovement(
    new PixelsMovement(UIContext::instance(), getSite(),
                       image, &mask2, "Paste"));

  setState(EditorStatePtr(new MovingPixelsState(this, NULL, pixelsMovement, NoHandle)));
}

void Editor::startSelectionTransformation(const gfx::Point& move, double angle)
{
  if (MovingPixelsState* movingPixels = dynamic_cast<MovingPixelsState*>(m_state.get())) {
    movingPixels->translate(move);
    if (std::fabs(angle) > 1e-5)
      movingPixels->rotate(angle);
  }
  else if (StandbyState* standby = dynamic_cast<StandbyState*>(m_state.get())) {
    standby->startSelectionTransformation(this, move, angle);
  }
}

void Editor::notifyScrollChanged()
{
  m_observers.notifyScrollChanged(this);
}

void Editor::play()
{
  ASSERT(m_state);
  if (!m_state)
    return;

  if (!dynamic_cast<PlayState*>(m_state.get()))
    setState(EditorStatePtr(new PlayState));
}

void Editor::stop()
{
  ASSERT(m_state);
  if (!m_state)
    return;

  if (dynamic_cast<PlayState*>(m_state.get()))
    backToPreviousState();
}

bool Editor::isPlaying() const
{
  return (dynamic_cast<PlayState*>(m_state.get()) != nullptr);
}

void Editor::showAnimationSpeedMultiplierPopup(bool withStopBehaviorOptions)
{
  double options[] = { 0.25, 0.5, 1.0, 1.5, 2.0, 3.0 };
  Menu menu;

  for (double option : options) {
    MenuItem* item = new MenuItem("Speed x" + base::convert_to<std::string>(option));
    item->Click.connect(Bind<void>(&Editor::setAnimationSpeedMultiplier, this, option));
    item->setSelected(m_aniSpeed == option);
    menu.addChild(item);
  }

  if (withStopBehaviorOptions) {
    menu.addChild(new MenuSeparator);
    MenuItem* item = new MenuItem("Rewind on Stop");
    item->Click.connect(
      []() {
        // Switch the "rewind_on_stop" option
        Preferences::instance().general.rewindOnStop(
          !Preferences::instance().general.rewindOnStop());
      });
    item->setSelected(Preferences::instance().general.rewindOnStop());
    menu.addChild(item);
  }

  menu.showPopup(ui::get_mouse_position());
}

double Editor::getAnimationSpeedMultiplier() const
{
  return m_aniSpeed;
}

void Editor::setAnimationSpeedMultiplier(double speed)
{
  m_aniSpeed = speed;
}

void Editor::showMouseCursor(CursorType cursorType)
{
  m_brushPreview.hide();
  ui::set_mouse_cursor(cursorType);
}

void Editor::showBrushPreview(const gfx::Point& screenPos)
{
  ui::set_mouse_cursor(kNoCursor);
  m_brushPreview.show(screenPos);
}

// static
ImageBufferPtr Editor::getRenderImageBuffer()
{
  return m_renderBuffer;
}

// static
gfx::Point Editor::calcExtraPadding(const Zoom& zoom)
{
  View* view = View::getView(this);
  if (view) {
    Rect vp = view->getViewportBounds();
    return gfx::Point(
      std::max<int>(vp.w/2, vp.w - zoom.apply(m_sprite->width())),
      std::max<int>(vp.h/2, vp.h - zoom.apply(m_sprite->height())));
  }
  else
    return gfx::Point(0, 0);
}

bool Editor::isMovingPixels() const
{
  return (dynamic_cast<MovingPixelsState*>(m_state.get()) != nullptr);
}

void Editor::dropMovingPixels()
{
  ASSERT(isMovingPixels());
  backToPreviousState();
}

} // namespace app
