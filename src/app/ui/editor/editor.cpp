// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/editor.h"

#include "app/app.h"
#include "app/color.h"
#include "app/color_picker.h"
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
#include "app/tools/active_tool.h"
#include "app/tools/ink.h"
#include "app/tools/tool.h"
#include "app/tools/tool_box.h"
#include "app/ui/color_bar.h"
#include "app/ui/context_bar.h"
#include "app/ui/editor/drawing_state.h"
#include "app/ui/editor/editor_customization_delegate.h"
#include "app/ui/editor/editor_decorator.h"
#include "app/ui/editor/glue.h"
#include "app/ui/editor/moving_pixels_state.h"
#include "app/ui/editor/pixels_movement.h"
#include "app/ui/editor/play_state.h"
#include "app/ui/editor/scrolling_state.h"
#include "app/ui/editor/standby_state.h"
#include "app/ui/editor/zooming_state.h"
#include "app/ui/main_window.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui/toolbar.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "base/chrono.h"
#include "base/convert_to.h"
#include "base/unique_ptr.h"
#include "doc/conversion_she.h"
#include "doc/doc.h"
#include "doc/document_event.h"
#include "doc/mask_boundaries.h"
#include "doc/slice.h"
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

// TODO these should be grouped in some kind of "performance counters"
static base::Chrono renderChrono;
static double renderElapsed = 0.0;

class EditorPreRenderImpl : public EditorPreRender {
public:
  EditorPreRenderImpl(Editor* editor, Image* image,
                      const Point& offset,
                      const Projection& proj)
    : m_editor(editor)
    , m_image(image)
    , m_offset(offset)
    , m_proj(proj) {
  }

  Editor* getEditor() override {
    return m_editor;
  }

  Image* getImage() override {
    return m_image;
  }

  void fillRect(const gfx::Rect& rect, uint32_t rgbaColor, int opacity) override
  {
    blend_rect(
      m_image,
      m_offset.x + m_proj.applyX(rect.x),
      m_offset.y + m_proj.applyY(rect.y),
      m_offset.x + m_proj.applyX(rect.x+rect.w) - 1,
      m_offset.y + m_proj.applyY(rect.y+rect.h) - 1, rgbaColor, opacity);
  }

private:
  Editor* m_editor;
  Image* m_image;
  Point m_offset;
  Projection m_proj;
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
    gfx::Rect bounds = m_editor->bounds();
    a.x -= bounds.x;
    a.y -= bounds.y;
    b.x -= bounds.x;
    b.y -= bounds.y;
    m_g->drawLine(screenColor, a, b);
  }

  void drawRectXor(const gfx::Rect& rc) override {
    gfx::Rect rc2 = m_editor->editorToScreen(rc);
    gfx::Rect bounds = m_editor->bounds();
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
  , m_layer(m_sprite->root()->firstLayer())
  , m_frame(frame_t(0))
  , m_docPref(Preferences::instance().document(document))
  , m_brushPreview(this)
  , m_lastDrawingPosition(-1, -1)
  , m_toolLoopModifiers(tools::ToolLoopModifiers::kNone)
  , m_padding(0, 0)
  , m_antsTimer(100, this)
  , m_antsOffset(0)
  , m_customizationDelegate(NULL)
  , m_docView(NULL)
  , m_flags(flags)
  , m_secondaryButton(false)
  , m_aniSpeed(1.0)
  , m_isPlaying(false)
  , m_showGuidesThisCel(nullptr)
  , m_tagFocusBand(-1)
{
  m_proj.setPixelRatio(m_sprite->pixelRatio());

  // Add the first state into the history.
  m_statesHistory.push(m_state);

  this->setFocusStop(true);

  App::instance()->activeToolManager()->add_observer(this);

  m_fgColorChangeConn =
    Preferences::instance().colorBar.fgColor.AfterChange.connect(
      base::Bind<void>(&Editor::onFgColorChange, this));

  m_contextBarBrushChangeConn =
    App::instance()->contextBar()->BrushChange.connect(
      base::Bind<void>(&Editor::onContextBarBrushChange, this));

  // Restore last site in preferences
  {
    frame_t preferredFrame = m_docPref.site.frame();
    if (preferredFrame >= 0 && preferredFrame <= m_sprite->lastFrame())
      setFrame(preferredFrame);

    LayerList layers = m_sprite->allBrowsableLayers();
    layer_t layerIndex = m_docPref.site.layer();
    if (layerIndex >= 0 && layerIndex < int(layers.size()))
      setLayer(layers[layerIndex]);
  }

  m_tiledConn = m_docPref.tiled.AfterChange.connect(base::Bind<void>(&Editor::invalidate, this));
  m_gridConn = m_docPref.grid.AfterChange.connect(base::Bind<void>(&Editor::invalidate, this));
  m_pixelGridConn = m_docPref.pixelGrid.AfterChange.connect(base::Bind<void>(&Editor::invalidate, this));
  m_bgConn = m_docPref.bg.AfterChange.connect(base::Bind<void>(&Editor::invalidate, this));
  m_onionskinConn = m_docPref.onionskin.AfterChange.connect(base::Bind<void>(&Editor::invalidate, this));
  m_symmetryModeConn = Preferences::instance().symmetryMode.enabled.AfterChange.connect(base::Bind<void>(&Editor::invalidateIfActive, this));
  m_showExtrasConn =
    m_docPref.show.AfterChange.connect(
      base::Bind<void>(&Editor::onShowExtrasChange, this));

  m_document->add_observer(this);

  m_state->onEnterState(this);
}

Editor::~Editor()
{
  if (m_document && m_sprite) {
    LayerList layers = m_sprite->allBrowsableLayers();
    layer_t layerIndex = doc::find_layer_index(layers, layer());

    m_docPref.site.frame(frame());
    m_docPref.site.layer(layerIndex);
  }

  m_observers.notifyDestroyEditor(this);
  m_document->remove_observer(this);
  App::instance()->activeToolManager()->remove_observer(this);

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

    // Save the current state into "m_deletedStates" just to keep a
    // reference to it to avoid delete it right now. We'll delete it
    // in the next Editor::onProcessMessage().
    //
    // This is necessary for PlayState because it removes itself
    // calling Editor::stop() from PlayState::onPlaybackTick(). If we
    // delete the PlayState inside the "Tick" timer signal, the
    // program will crash (because we're iterating the
    // PlayState::m_playTimer slots).
    m_deletedStates.push(m_state);

    m_statesHistory.pop();
    m_state = m_statesHistory.top();
  }

  ASSERT(m_state);

  // Change to the new state.
  m_state->onEnterState(this);

  // Notify observers
  m_observers.notifyStateChanged(this);

  // Redraw layer edges
  if (m_docPref.show.layerEdges())
    invalidate();

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

void Editor::getInvalidDecoratoredRegion(gfx::Region& region)
{
  // Remove decorated region that cannot be just moved because it
  // must be redrawn in another position when the Editor's scroll
  // changes (e.g. symmetry handles).
  if ((m_flags & kShowDecorators) && m_decorator)
    m_decorator->getInvalidDecoratoredRegion(this, region);
}

void Editor::setLayer(const Layer* layer)
{
  bool changed = (m_layer != layer);

  m_observers.notifyBeforeLayerChanged(this);
  m_layer = const_cast<Layer*>(layer);
  m_observers.notifyAfterLayerChanged(this);

  if (m_document && changed) {
    if (// If the onion skinning depends on the active layer
        m_docPref.onionskin.currentLayer() ||
        // If the user want to see the active layer edges...
        m_docPref.show.layerEdges() ||
        // If there is a different opacity for nonactive-layers
        Preferences::instance().experimental.nonactiveLayersOpacity() < 255 ||
        // If the automatic cel guides are visible...
        m_showGuidesThisCel) {
      // We've to redraw the whole editor
      invalidate();
    }
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

void Editor::setZoom(const render::Zoom& zoom)
{
  if (m_proj.zoom() != zoom) {
    m_proj.setZoom(zoom);
    notifyZoomChanged();
  }
  else {
    // Just copy the zoom as the internal "Zoom::m_internalScale"
    // value might be different and we want to keep this value updated
    // for better zooming experience in StateWithWheelBehavior.
    m_proj.setZoom(zoom);
  }
}

void Editor::setDefaultScroll()
{
  View* view = View::getView(this);
  Rect vp = view->viewportBounds();

  setEditorScroll(
    gfx::Point(
      m_padding.x - vp.w/2 + m_proj.applyX(m_sprite->width())/2,
      m_padding.y - vp.h/2 + m_proj.applyY(m_sprite->height())/2));
}

void Editor::setScrollAndZoomToFitScreen()
{
  View* view = View::getView(this);
  Rect vp = view->viewportBounds();
  Zoom zoom = m_proj.zoom();

  if (float(vp.w) / float(m_sprite->width()) <
      float(vp.h) / float(m_sprite->height())) {
    if (vp.w < m_proj.applyX(m_sprite->width())) {
      while (vp.w < m_proj.applyX(m_sprite->width())) {
        if (!zoom.out())
          break;
        m_proj.setZoom(zoom);
      }
    }
    else if (vp.w > m_proj.applyX(m_sprite->width())) {
      bool out = true;
      while (vp.w > m_proj.applyX(m_sprite->width())) {
        if (!zoom.in()) {
          out = false;
          break;
        }
        m_proj.setZoom(zoom);
      }
      if (out) {
        zoom.out();
        m_proj.setZoom(zoom);
      }
    }
  }
  else {
    if (vp.h < m_proj.applyY(m_sprite->height())) {
      while (vp.h < m_proj.applyY(m_sprite->height())) {
        if (!zoom.out())
          break;
        m_proj.setZoom(zoom);
      }
    }
    else if (vp.h > m_proj.applyY(m_sprite->height())) {
      bool out = true;
      while (vp.h > m_proj.applyY(m_sprite->height())) {
        if (!zoom.in()) {
          out = false;
          break;
        }
        m_proj.setZoom(zoom);
      }
      if (out) {
        zoom.out();
        m_proj.setZoom(zoom);
      }
    }
  }

  updateEditor();
  setEditorScroll(
    gfx::Point(
      m_padding.x - vp.w/2 + m_proj.applyX(m_sprite->width())/2,
      m_padding.y - vp.h/2 + m_proj.applyY(m_sprite->height())/2));
}

// Sets the scroll position of the editor
void Editor::setEditorScroll(const gfx::Point& scroll)
{
  View::getView(this)->setViewScroll(scroll);
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
  rc = m_proj.apply(rc);

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
      gfx::Rect expose = m_proj.remove(rc);

      // If the zoom level is less than 100%, we add extra pixels to
      // the exposed area. Those pixels could be shown in the
      // rendering process depending on each cel position.
      // E.g. when we are drawing in a cel with position < (0,0)
      if (m_proj.scaleX() < 1.0)
        expose.enlargeXW(int(1./m_proj.scaleX()));
      // If the zoom level is more than %100 we add an extra pixel to
      // expose just in case the zoom requires to display it.  Note:
      // this is really necessary to avoid showing invalid destination
      // areas in ToolLoopImpl.
      else if (m_proj.scaleX() > 1.0)
        expose.enlargeXW(1);

      if (m_proj.scaleY() < 1.0)
        expose.enlargeYH(int(1./m_proj.scaleY()));
      else if (m_proj.scaleY() > 1.0)
        expose.enlargeYH(1);

      m_document->notifyExposeSpritePixels(m_sprite, gfx::Region(expose));
    }

    // Create a temporary RGB bitmap to draw all to it
    rendered.reset(Image::create(IMAGE_RGB, rc.w, rc.h, m_renderBuffer));

    m_renderEngine.setRefLayersVisiblity(true);
    m_renderEngine.setSelectedLayer(m_layer);
    if (m_flags & Editor::kUseNonactiveLayersOpacityWhenEnabled)
      m_renderEngine.setNonactiveLayersOpacity(Preferences::instance().experimental.nonactiveLayersOpacity());
    else
      m_renderEngine.setNonactiveLayersOpacity(255);
    m_renderEngine.setProjection(m_proj);
    m_renderEngine.setupBackground(m_document, rendered->pixelFormat());
    m_renderEngine.disableOnionskin();

    if ((m_flags & kShowOnionskin) == kShowOnionskin) {
      if (m_docPref.onionskin.active()) {
        OnionskinOptions opts(
          (m_docPref.onionskin.type() == app::gen::OnionskinType::MERGE ?
           render::OnionskinType::MERGE:
           (m_docPref.onionskin.type() == app::gen::OnionskinType::RED_BLUE_TINT ?
            render::OnionskinType::RED_BLUE_TINT:
            render::OnionskinType::NONE)));

        opts.position(m_docPref.onionskin.position());
        opts.prevFrames(m_docPref.onionskin.prevFrames());
        opts.nextFrames(m_docPref.onionskin.nextFrames());
        opts.opacityBase(m_docPref.onionskin.opacityBase());
        opts.opacityStep(m_docPref.onionskin.opacityStep());
        opts.layer(m_docPref.onionskin.currentLayer() ? m_layer: nullptr);

        FrameTag* tag = nullptr;
        if (m_docPref.onionskin.loopTag())
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

    m_renderEngine.renderSprite(
      rendered, m_sprite, m_frame, gfx::Clip(0, 0, rc));

    m_renderEngine.removeExtraImage();
  }
  catch (const std::exception& e) {
    Console::showException(e);
  }

  if (rendered) {
    // Pre-render decorator.
    if ((m_flags & kShowDecorators) && m_decorator) {
      EditorPreRenderImpl preRender(this, rendered,
                                    Point(-rc.x, -rc.y), m_proj);
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
  if (m_proj.scaleX() < 1.0) rc.w += int(1./m_proj.scaleX());
  if (m_proj.scaleY() < 1.0) rc.h += int(1./m_proj.scaleY());

  gfx::Rect client = clientBounds();
  gfx::Rect spriteRect(
    client.x + m_padding.x,
    client.y + m_padding.y,
    m_proj.applyX(m_sprite->width()),
    m_proj.applyY(m_sprite->height()));
  gfx::Rect enclosingRect = spriteRect;

  // Draw the main sprite at the center.
  drawOneSpriteUnclippedRect(g, rc, 0, 0);

  gfx::Region outside(client);
  outside.createSubtraction(outside, gfx::Region(spriteRect));

  // Document preferences
  if (int(m_docPref.tiled.mode()) & int(filters::TiledMode::X_AXIS)) {
    drawOneSpriteUnclippedRect(g, rc, -spriteRect.w, 0);
    drawOneSpriteUnclippedRect(g, rc, +spriteRect.w, 0);

    enclosingRect = gfx::Rect(spriteRect.x-spriteRect.w, spriteRect.y, spriteRect.w*3, spriteRect.h);
    outside.createSubtraction(outside, gfx::Region(enclosingRect));
  }

  if (int(m_docPref.tiled.mode()) & int(filters::TiledMode::Y_AXIS)) {
    drawOneSpriteUnclippedRect(g, rc, 0, -spriteRect.h);
    drawOneSpriteUnclippedRect(g, rc, 0, +spriteRect.h);

    enclosingRect = gfx::Rect(spriteRect.x, spriteRect.y-spriteRect.h, spriteRect.w, spriteRect.h*3);
    outside.createSubtraction(outside, gfx::Region(enclosingRect));
  }

  if (m_docPref.tiled.mode() == filters::TiledMode::BOTH) {
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
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
  if (m_flags & kShowOutside) {
    g->fillRegion(theme->colors.editorFace(), outside);
  }

  // Grids & slices
  {
    // Clipping
    gfx::Rect cliprc = editorToScreen(rc).offset(-bounds().origin());
    cliprc = cliprc.createIntersection(spriteRect);
    if (!cliprc.isEmpty()) {
      IntersectClip clip(g, cliprc);

      // Draw the pixel grid
      if ((m_proj.zoom().scale() > 2.0) && m_docPref.show.pixelGrid()) {
        int alpha = m_docPref.pixelGrid.opacity();

        if (m_docPref.pixelGrid.autoOpacity()) {
          alpha = int(alpha * (m_proj.zoom().scale()-2.) / (16.-2.));
          alpha = MID(0, alpha, 255);
        }

        drawGrid(g, enclosingRect, Rect(0, 0, 1, 1),
          m_docPref.pixelGrid.color(), alpha);
      }

      // Draw the grid
      if (m_docPref.show.grid()) {
        gfx::Rect gridrc = m_docPref.grid.bounds();
        if (m_proj.applyX(gridrc.w) > 2 &&
            m_proj.applyY(gridrc.h) > 2) {
          int alpha = m_docPref.grid.opacity();

          if (m_docPref.grid.autoOpacity()) {
            double len = (m_proj.applyX(gridrc.w) +
                          m_proj.applyY(gridrc.h)) / 2.;
            alpha = int(alpha * len / 32.);
            alpha = MID(0, alpha, 255);
          }

          if (alpha > 8)
            drawGrid(g, enclosingRect, m_docPref.grid.bounds(),
              m_docPref.grid.color(), alpha);
        }
      }

      // Draw slices
      if (m_docPref.show.slices())
        drawSlices(g);
    }
  }

  // Symmetry mode
  if (isActive() &&
      (m_flags & Editor::kShowSymmetryLine) &&
      Preferences::instance().symmetryMode.enabled()) {
    int mode = int(m_docPref.symmetry.mode());
    if (mode & int(app::gen::SymmetryMode::HORIZONTAL)) {
      double x = m_docPref.symmetry.xAxis();
      if (x > 0) {
        gfx::Color color = color_utils::color_for_ui(m_docPref.grid.color());
        g->drawVLine(color,
                     spriteRect.x + m_proj.applyX<double>(x),
                     enclosingRect.y,
                     enclosingRect.h);
      }
    }
    if (mode & int(app::gen::SymmetryMode::VERTICAL)) {
      double y = m_docPref.symmetry.yAxis();
      if (y > 0) {
        gfx::Color color = color_utils::color_for_ui(m_docPref.grid.color());
        g->drawHLine(color,
                     enclosingRect.x,
                     spriteRect.y + m_proj.applyY<double>(y),
                     enclosingRect.w);
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

  // Draw active layer/cel edges
  bool showGuidesThisCel = this->showAutoCelGuides();
  if ((m_docPref.show.layerEdges() || showGuidesThisCel) &&
      // Show layer edges only on "standby" like states where brush
      // preview is shown (e.g. with this we avoid to showing the
      // edges in states like DrawingState, etc.).
      m_state->requireBrushPreview()) {
    Cel* cel = (m_layer ? m_layer->cel(m_frame): nullptr);
    if (cel) {
      drawCelBounds(
        g, cel,
        color_utils::color_for_ui(Preferences::instance().guides.layerEdgesColor()));

      if (showGuidesThisCel &&
          m_showGuidesThisCel != cel)
        drawCelGuides(g, cel, m_showGuidesThisCel);
    }
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
  GraphicsPtr editorGraphics = getGraphics(clientBounds());

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
  if ((m_flags & kShowMask) == 0 ||
      !m_docPref.show.selectionEdges())
    return;

  ASSERT(m_document->getMaskBoundaries());

  int x = m_padding.x;
  int y = m_padding.y;

  for (const auto& seg : *m_document->getMaskBoundaries()) {
    CheckedDrawMode checked(g, m_antsOffset,
                            gfx::rgba(0, 0, 0, 255),
                            gfx::rgba(255, 255, 255, 255));
    gfx::Rect bounds = m_proj.apply(seg.bounds());

    if (m_proj.scaleX() >= 1.0) {
      if (!seg.open() && seg.vertical())
        --bounds.x;
    }

    if (m_proj.scaleY() >= 1.0) {
      if (!seg.open() && !seg.vertical())
        --bounds.y;
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
    region.offset(-bounds().origin());

    HideBrushPreview hide(m_brushPreview);
    GraphicsPtr g = getGraphics(clientBounds());

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
  gfx::Rect bounds = this->bounds();
  grid.offset(-bounds.origin());

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

void Editor::drawSlices(ui::Graphics* g)
{
  if ((m_flags & kShowSlices) == 0)
    return;

  if (!isVisible() || !m_document)
    return;

  for (auto slice : m_sprite->slices()) {
    auto key = slice->getByFrame(m_frame);
    if (!key)
      continue;

    doc::color_t docColor = slice->userData().color();
    gfx::Color color = gfx::rgba(doc::rgba_getr(docColor),
                                 doc::rgba_getg(docColor),
                                 doc::rgba_getb(docColor),
                                 doc::rgba_geta(docColor));
    gfx::Rect out =
      editorToScreen(key->bounds())
               .offset(-bounds().origin());

    // Center slices
    if (key->hasCenter()) {
      gfx::Rect in =
        editorToScreen(gfx::Rect(key->center()).offset(key->bounds().origin()))
        .offset(-bounds().origin());

      auto in_color = gfx::rgba(gfx::getr(color),
                                gfx::getg(color),
                                gfx::getb(color),
                                doc::rgba_geta(docColor)/4);
      if (in.y > out.y && in.y < out.y2())
        g->drawHLine(in_color, out.x, in.y, out.w);
      if (in.y2() > out.y && in.y2() < out.y2())
        g->drawHLine(in_color, out.x, in.y2(), out.w);
      if (in.x > out.x && in.x < out.x2())
        g->drawVLine(in_color, in.x, out.y, out.h);
      if (in.x2() > out.x && in.x2() < out.x2())
        g->drawVLine(in_color, in.x2(), out.y, out.h);
    }

    // Pivot
    if (key->hasPivot()) {
      gfx::Rect in =
        editorToScreen(gfx::Rect(key->pivot(), gfx::Size(1, 1)).offset(key->bounds().origin()))
        .offset(-bounds().origin());

      auto in_color = gfx::rgba(gfx::getr(color),
                                gfx::getg(color),
                                gfx::getb(color),
                                doc::rgba_geta(docColor)/4);
      g->drawRect(in_color, in);
    }

    g->drawRect(color, out);
  }
}

void Editor::drawCelBounds(ui::Graphics* g, const Cel* cel, const gfx::Color color)
{
  g->drawRect(color, getCelScreenBounds(cel));
}

void Editor::drawCelGuides(ui::Graphics* g, const Cel* cel, const Cel* mouseCel)
{
  gfx::Rect
    sprCelBounds = cel->bounds(),
    scrCelBounds = getCelScreenBounds(cel),
    scrCmpBounds, sprCmpBounds;
  if (mouseCel) {
    scrCmpBounds = getCelScreenBounds(mouseCel);
    sprCmpBounds = mouseCel->bounds();

    drawCelBounds(
      g, mouseCel,
      color_utils::color_for_ui(Preferences::instance().guides.autoGuidesColor()));
  }
  // Use whole canvas
  else {
    sprCmpBounds = m_sprite->bounds();
    scrCmpBounds = editorToScreen(sprCmpBounds).offset(gfx::Point(-bounds().origin()));
  }

  const int midX = scrCelBounds.x+scrCelBounds.w/2;
  const int midY = scrCelBounds.y+scrCelBounds.h/2;

  if (sprCelBounds.x2() < sprCmpBounds.x) {
    drawCelHGuide(g,
                  sprCelBounds.x2(), sprCmpBounds.x,
                  scrCelBounds.x2(), scrCmpBounds.x, midY,
                  scrCelBounds, scrCmpBounds, scrCmpBounds.x);
  }
  else if (sprCelBounds.x > sprCmpBounds.x2()) {
    drawCelHGuide(g,
                  sprCmpBounds.x2(), sprCelBounds.x,
                  scrCmpBounds.x2(), scrCelBounds.x, midY,
                  scrCelBounds, scrCmpBounds, scrCmpBounds.x2()-1);
  }
  else {
    if (sprCelBounds.x != sprCmpBounds.x &&
        sprCelBounds.x2() != sprCmpBounds.x) {
      drawCelHGuide(g,
                    sprCmpBounds.x, sprCelBounds.x,
                    scrCmpBounds.x, scrCelBounds.x, midY,
                    scrCelBounds, scrCmpBounds, scrCmpBounds.x);
    }
    if (sprCelBounds.x != sprCmpBounds.x2() &&
        sprCelBounds.x2() != sprCmpBounds.x2()) {
      drawCelHGuide(g,
                    sprCmpBounds.x2(), sprCelBounds.x2(),
                    scrCmpBounds.x2(), scrCelBounds.x2(), midY,
                    scrCelBounds, scrCmpBounds, scrCmpBounds.x2()-1);
    }
  }

  if (sprCelBounds.y2() < sprCmpBounds.y) {
    drawCelVGuide(g,
                  sprCelBounds.y2(), sprCmpBounds.y,
                  scrCelBounds.y2(), scrCmpBounds.y, midX,
                  scrCelBounds, scrCmpBounds, scrCmpBounds.y);
  }
  else if (sprCelBounds.y > sprCmpBounds.y2()) {
    drawCelVGuide(g,
                  sprCmpBounds.y2(), sprCelBounds.y,
                  scrCmpBounds.y2(), scrCelBounds.y, midX,
                  scrCelBounds, scrCmpBounds, scrCmpBounds.y2()-1);
  }
  else {
    if (sprCelBounds.y != sprCmpBounds.y &&
        sprCelBounds.y2() != sprCmpBounds.y) {
      drawCelVGuide(g,
                    sprCmpBounds.y, sprCelBounds.y,
                    scrCmpBounds.y, scrCelBounds.y, midX,
                    scrCelBounds, scrCmpBounds, scrCmpBounds.y);
    }
    if (sprCelBounds.y != sprCmpBounds.y2() &&
        sprCelBounds.y2() != sprCmpBounds.y2()) {
      drawCelVGuide(g,
                    sprCmpBounds.y2(), sprCelBounds.y2(),
                    scrCmpBounds.y2(), scrCelBounds.y2(), midX,
                    scrCelBounds, scrCmpBounds, scrCmpBounds.y2()-1);
    }
  }
}

void Editor::drawCelHGuide(ui::Graphics* g,
                           const int sprX1, const int sprX2,
                           const int scrX1, const int scrX2, const int scrY,
                           const gfx::Rect& scrCelBounds, const gfx::Rect& scrCmpBounds,
                           const int dottedX)
{
  gfx::Color color = color_utils::color_for_ui(Preferences::instance().guides.autoGuidesColor());
  g->drawHLine(color, scrX1, scrY, scrX2 - scrX1);

  // Vertical guide to touch the horizontal line
  {
    CheckedDrawMode checked(g, 0, color, gfx::ColorNone);

    if (scrY < scrCmpBounds.y)
      g->drawVLine(color, dottedX, scrCelBounds.y, scrCmpBounds.y - scrCelBounds.y);
    else if (scrY > scrCmpBounds.y2())
      g->drawVLine(color, dottedX, scrCmpBounds.y2(), scrCelBounds.y2() - scrCmpBounds.y2());
  }

  auto text = base::convert_to<std::string>(ABS(sprX2 - sprX1)) + "px";
  const int textW = Graphics::measureUITextLength(text, font());
  g->drawText(text,
              color_utils::blackandwhite_neg(color), color,
              gfx::Point((scrX1+scrX2)/2-textW/2, scrY-textHeight()));
}

void Editor::drawCelVGuide(ui::Graphics* g,
                           const int sprY1, const int sprY2,
                           const int scrY1, const int scrY2, const int scrX,
                           const gfx::Rect& scrCelBounds, const gfx::Rect& scrCmpBounds,
                           const int dottedY)
{
  gfx::Color color = color_utils::color_for_ui(Preferences::instance().guides.autoGuidesColor());
  g->drawVLine(color, scrX, scrY1, scrY2 - scrY1);

  // Horizontal guide to touch the vertical line
  {
    CheckedDrawMode checked(g, 0, color, gfx::ColorNone);

    if (scrX < scrCmpBounds.x)
      g->drawHLine(color, scrCelBounds.x, dottedY, scrCmpBounds.x - scrCelBounds.x);
    else if (scrX > scrCmpBounds.x2())
      g->drawHLine(color, scrCmpBounds.x2(), dottedY, scrCelBounds.x2() - scrCmpBounds.x2());
  }

  auto text = base::convert_to<std::string>(ABS(sprY2 - sprY1)) + "px";
  g->drawText(text,
              color_utils::blackandwhite_neg(color), color,
              gfx::Point(scrX, (scrY1+scrY2)/2-textHeight()/2));
}

gfx::Rect Editor::getCelScreenBounds(const Cel* cel)
{
  gfx::Rect layerEdges;
  if (m_layer->isReference()) {
    layerEdges = editorToScreenF(cel->boundsF()).offset(gfx::PointF(-bounds().origin()));
  }
  else {
    layerEdges = editorToScreen(cel->bounds()).offset(-bounds().origin());
  }
  return layerEdges;
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
      manager()->flipDisplay();
      m_document->setExtraCel(oldExtraCel);
    }

    invalidate();
  }
}

gfx::Point Editor::autoScroll(MouseMessage* msg, AutoScroll dir)
{
  gfx::Point mousePos = msg->position();
  if (!Preferences::instance().editor.autoScroll())
    return mousePos;

  // Hide the brush preview
  //HideBrushPreview hide(editor->brushPreview());
  View* view = View::getView(this);
  gfx::Rect vp = view->viewportBounds();

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

    gfx::Point scroll = view->viewScroll();
    if (dir == AutoScroll::MouseDir) {
      scroll += delta;
    }
    else {
      scroll -= deltaScroll;
    }
    setEditorScroll(scroll);

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

tools::Tool* Editor::getCurrentEditorTool()
{
  return App::instance()->activeTool();
}

tools::Ink* Editor::getCurrentEditorInk()
{
  tools::Ink* ink = m_state->getStateInk();
  if (ink)
    return ink;
  else
    return App::instance()->activeToolManager()->activeInk();
}

bool Editor::isAutoSelectLayer() const
{
  return App::instance()->contextBar()->isAutoSelectLayer();
}

gfx::Point Editor::screenToEditor(const gfx::Point& pt)
{
  View* view = View::getView(this);
  Rect vp = view->viewportBounds();
  Point scroll = view->viewScroll();
  return gfx::Point(
    m_proj.removeX(pt.x - vp.x + scroll.x - m_padding.x),
    m_proj.removeY(pt.y - vp.y + scroll.y - m_padding.y));
}

gfx::PointF Editor::screenToEditorF(const gfx::Point& pt)
{
  View* view = View::getView(this);
  Rect vp = view->viewportBounds();
  Point scroll = view->viewScroll();
  return gfx::PointF(
    m_proj.removeX<double>(pt.x - vp.x + scroll.x - m_padding.x),
    m_proj.removeY<double>(pt.y - vp.y + scroll.y - m_padding.y));
}

Point Editor::editorToScreen(const gfx::Point& pt)
{
  View* view = View::getView(this);
  Rect vp = view->viewportBounds();
  Point scroll = view->viewScroll();
  return Point(
    (vp.x - scroll.x + m_padding.x + m_proj.applyX(pt.x)),
    (vp.y - scroll.y + m_padding.y + m_proj.applyY(pt.y)));
}

gfx::PointF Editor::editorToScreenF(const gfx::PointF& pt)
{
  View* view = View::getView(this);
  Rect vp = view->viewportBounds();
  Point scroll = view->viewScroll();
  return PointF(
    (vp.x - scroll.x + m_padding.x + m_proj.applyX<double>(pt.x)),
    (vp.y - scroll.y + m_padding.y + m_proj.applyY<double>(pt.y)));
}

Rect Editor::screenToEditor(const Rect& rc)
{
  return gfx::Rect(
    screenToEditor(rc.origin()),
    screenToEditor(rc.point2()));
}

Rect Editor::editorToScreen(const Rect& rc)
{
  return gfx::Rect(
    editorToScreen(rc.origin()),
    editorToScreen(rc.point2()));
}

gfx::RectF Editor::editorToScreenF(const gfx::RectF& rc)
{
  return gfx::RectF(
    editorToScreenF(rc.origin()),
    editorToScreenF(rc.point2()));
}

void Editor::add_observer(EditorObserver* observer)
{
  m_observers.add_observer(observer);
}

void Editor::remove_observer(EditorObserver* observer)
{
  m_observers.remove_observer(observer);
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
  Rect vp = view->viewportBounds();
  vp = screenToEditor(vp);

  return vp.createIntersection(m_sprite->bounds());
}

// Changes the scroll to see the given point as the center of the editor.
void Editor::centerInSpritePoint(const gfx::Point& spritePos)
{
  HideBrushPreview hide(m_brushPreview);
  View* view = View::getView(this);
  Rect vp = view->viewportBounds();

  gfx::Point scroll(
    m_padding.x - (vp.w/2) + m_proj.applyX(1)/2 + m_proj.applyX(spritePos.x),
    m_padding.y - (vp.h/2) + m_proj.applyY(1)/2 + m_proj.applyY(spritePos.y));

  updateEditor();
  setEditorScroll(scroll);
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
  if (m_customizationDelegate && !hasCapture()) {
    auto activeToolManager = App::instance()->activeToolManager();
    tools::Tool* selectedTool = activeToolManager->selectedTool();

    // Don't change quicktools if we are in a selection tool and using
    // the selection modifiers.
    if (selectedTool->getInk(0)->isSelection() &&
        int(m_customizationDelegate->getPressedKeyAction(KeyContext::SelectionTool)) != 0)
      return;

    tools::Tool* newQuicktool =
      m_customizationDelegate->getQuickTool(selectedTool);

    // Check if the current state accept the given quicktool.
    if (newQuicktool && !m_state->acceptQuickTool(newQuicktool))
      return;

    activeToolManager
      ->newQuickToolSelectedFromEditor(newQuicktool);
  }
}

void Editor::updateToolByTipProximity(ui::PointerType pointerType)
{
  auto activeToolManager = App::instance()->activeToolManager();

  if (pointerType == ui::PointerType::Eraser) {
    activeToolManager->eraserTipProximity();
  }
  else {
    activeToolManager->regularTipProximity();
  }
}

void Editor::updateToolLoopModifiersIndicators()
{
  int modifiers = int(tools::ToolLoopModifiers::kNone);
  const bool autoSelectLayer = isAutoSelectLayer();
  bool newAutoSelectLayer = autoSelectLayer;
  KeyAction action;

  if (m_customizationDelegate) {
    // When the mouse is captured, is when we are scrolling, or
    // drawing, or moving, or selecting, etc. So several
    // parameters/tool-loop-modifiers are static.
    if (hasCapture()) {
      modifiers |= (int(m_toolLoopModifiers) &
                    (int(tools::ToolLoopModifiers::kReplaceSelection) |
                     int(tools::ToolLoopModifiers::kAddSelection) |
                     int(tools::ToolLoopModifiers::kSubtractSelection)));

      // Shape tools (line, curves, rectangles, etc.)
      action = m_customizationDelegate->getPressedKeyAction(KeyContext::ShapeTool);
      if (int(action & KeyAction::MoveOrigin))
        modifiers |= int(tools::ToolLoopModifiers::kMoveOrigin);
      if (int(action & KeyAction::SquareAspect))
        modifiers |= int(tools::ToolLoopModifiers::kSquareAspect);
      if (int(action & KeyAction::DrawFromCenter))
        modifiers |= int(tools::ToolLoopModifiers::kFromCenter);
    }
    else {
      // We update the selection mode only if we're not selecting.
      action = m_customizationDelegate->getPressedKeyAction(KeyContext::SelectionTool);

      gen::SelectionMode mode = Preferences::instance().selection.mode();
      if (int(action & KeyAction::SubtractSelection) ||
          // Don't use "subtract" mode if the selection was activated
          // with the "right click mode = a selection-like tool"
          (m_secondaryButton &&
           App::instance()->activeToolManager()->selectedTool() &&
           App::instance()->activeToolManager()->selectedTool()->getInk(0)->isSelection())) {
        mode = gen::SelectionMode::SUBTRACT;
      }
      else if (int(action & KeyAction::AddSelection)) {
        mode = gen::SelectionMode::ADD;
      }
      switch (mode) {
        case gen::SelectionMode::DEFAULT:  modifiers |= int(tools::ToolLoopModifiers::kReplaceSelection);  break;
        case gen::SelectionMode::ADD:      modifiers |= int(tools::ToolLoopModifiers::kAddSelection);      break;
        case gen::SelectionMode::SUBTRACT: modifiers |= int(tools::ToolLoopModifiers::kSubtractSelection); break;
      }

      // For move tool
      action = m_customizationDelegate->getPressedKeyAction(KeyContext::MoveTool);
      if (int(action & KeyAction::AutoSelectLayer))
        newAutoSelectLayer = true;
      else
        newAutoSelectLayer = Preferences::instance().editor.autoSelectLayer();
    }
  }

  ContextBar* ctxBar = App::instance()->contextBar();

  if (int(m_toolLoopModifiers) != modifiers) {
    m_toolLoopModifiers = tools::ToolLoopModifiers(modifiers);

    // TODO the contextbar should be a observer of the current editor
    ctxBar->updateToolLoopModifiersIndicators(m_toolLoopModifiers);

    if (auto drawingState = dynamic_cast<DrawingState*>(m_state.get())) {
      drawingState->notifyToolLoopModifiersChange(this);
    }
  }

  if (autoSelectLayer != newAutoSelectLayer)
    ctxBar->updateAutoSelectLayer(newAutoSelectLayer);
}

app::Color Editor::getColorByPosition(const gfx::Point& mousePos)
{
  Site site = getSite();
  if (site.sprite()) {
    gfx::PointF editorPos = screenToEditorF(mousePos);

    ColorPicker picker;
    picker.pickColor(site, editorPos, m_proj,
                     ColorPicker::FromComposition);
    return picker.color();
  }
  else
    return app::Color::fromMask();
}

//////////////////////////////////////////////////////////////////////
// Message handler for the editor

bool Editor::onProcessMessage(Message* msg)
{
  // Delete states
  if (!m_deletedStates.empty())
    m_deletedStates.clear();

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
      updateToolLoopModifiersIndicators();
      updateQuicktool();
      break;

    case kMouseLeaveMessage:
      m_brushPreview.hide();
      StatusBar::instance()->clearText();
      break;

    case kMouseDownMessage:
      if (m_sprite) {
        MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);

        m_oldPos = mouseMsg->position();
        updateToolByTipProximity(mouseMsg->pointerType());
        updateAutoCelGuides(msg);

        // Only when we right-click with the regular "paint bg-color
        // right-click mode" we will mark indicate that the secondary
        // button was used (m_secondaryButton == true).
        if (mouseMsg->right() && !m_secondaryButton) {
          m_secondaryButton = true;
        }

        updateToolLoopModifiersIndicators();
        updateQuicktool();
        setCursor(mouseMsg->position());

        App::instance()->activeToolManager()
          ->pressButton(pointer_from_msg(this, mouseMsg));

        EditorStatePtr holdState(m_state);
        return m_state->onMouseDown(this, mouseMsg);
      }
      break;

    case kMouseMoveMessage:
      if (m_sprite) {
        EditorStatePtr holdState(m_state);
        MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);

        updateToolByTipProximity(mouseMsg->pointerType());
        updateAutoCelGuides(msg);

        return m_state->onMouseMove(this, static_cast<MouseMessage*>(msg));
      }
      break;

    case kMouseUpMessage:
      if (m_sprite) {
        EditorStatePtr holdState(m_state);
        MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
        bool result = m_state->onMouseUp(this, mouseMsg);

        updateToolByTipProximity(mouseMsg->pointerType());
        updateAutoCelGuides(msg);

        if (!hasCapture()) {
          App::instance()->activeToolManager()->releaseButtons();
          m_secondaryButton = false;

          updateToolLoopModifiersIndicators();
          updateQuicktool();
          setCursor(mouseMsg->position());
        }

        if (result)
          return true;
      }
      break;

    case kDoubleClickMessage:
      if (m_sprite) {
        MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
        EditorStatePtr holdState(m_state);

        updateToolByTipProximity(mouseMsg->pointerType());

        bool used = m_state->onDoubleClick(this, mouseMsg);
        if (used)
          return true;
      }
      break;

    case kTouchMagnifyMessage:
      if (m_sprite) {
        EditorStatePtr holdState(m_state);
        return m_state->onTouchMagnify(this, static_cast<TouchMessage*>(msg));
      }
      break;

    case kKeyDownMessage:
      if (m_sprite) {
        EditorStatePtr holdState(m_state);
        bool used = m_state->onKeyDown(this, static_cast<KeyMessage*>(msg));

        updateToolLoopModifiersIndicators();
        updateAutoCelGuides(msg);
        if (hasMouse()) {
          updateQuicktool();
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

        updateToolLoopModifiersIndicators();
        updateAutoCelGuides(msg);
        if (hasMouse()) {
          updateQuicktool();
          setCursor(ui::get_mouse_position());
        }

        if (used)
          return true;
      }
      break;

    case kFocusLeaveMessage:
      // As we use keys like Space-bar as modifier, we can clear the
      // keyboard buffer when we lost the focus.
      she::instance()->clearKeyboardBuffer();
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

void Editor::onSizeHint(SizeHintEvent& ev)
{
  gfx::Size sz(0, 0);

  if (m_sprite) {
    gfx::Point padding = calcExtraPadding(m_proj);
    sz.w = m_proj.applyX(m_sprite->width()) + padding.x*2;
    sz.h = m_proj.applyY(m_sprite->height()) + padding.y*2;
  }
  else {
    sz.w = 4;
    sz.h = 4;
  }
  ev.setSizeHint(sz);
}

void Editor::onResize(ui::ResizeEvent& ev)
{
  Widget::onResize(ev);
  m_padding = calcExtraPadding(m_proj);
}

void Editor::onPaint(ui::PaintEvent& ev)
{
  HideBrushPreview hide(m_brushPreview);
  Graphics* g = ev.graphics();
  gfx::Rect rc = clientBounds();
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());

  // Editor without sprite
  if (!m_sprite) {
    g->fillRect(theme->colors.editorFace(), rc);
  }
  // Editor with sprite
  else {
    try {
      // Lock the sprite to read/render it. We wait 1/4 secs in case
      // the background thread is making a backup.
      DocumentReader documentReader(m_document, 250);

      // Draw the sprite in the editor
      renderChrono.reset();
      drawSpriteUnclippedRect(g, gfx::Rect(0, 0, m_sprite->width(), m_sprite->height()));
      renderElapsed = renderChrono.elapsed();

      // Show performance stats (TODO show performance stats in other widget)
      if (Preferences::instance().perf.showRenderTime()) {
        View* view = View::getView(this);
        gfx::Rect vp = view->viewportBounds();
        char buf[128];
        sprintf(buf, "%.3f", renderElapsed);
        g->drawText(
          buf,
          gfx::rgba(255, 255, 255, 255),
          gfx::rgba(0, 0, 0, 255),
          vp.origin() - bounds().origin());
      }

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
      defer_invalid_rect(g->getClipBounds().offset(bounds().origin()));
    }
  }
}

void Editor::onInvalidateRegion(const gfx::Region& region)
{
  Widget::onInvalidateRegion(region);
  m_brushPreview.invalidateRegion(region);
}

// When the current tool is changed
void Editor::onActiveToolChange(tools::Tool* tool)
{
  m_state->onActiveToolChange(this, tool);
  updateStatusBar();
}

void Editor::onFgColorChange()
{
  m_brushPreview.redraw();
}

void Editor::onContextBarBrushChange()
{
  m_brushPreview.redraw();
}

void Editor::onShowExtrasChange()
{
  invalidate();
}

void Editor::onExposeSpritePixels(doc::DocumentEvent& ev)
{
  if (m_state && ev.sprite() == m_sprite)
    m_state->onExposeSpritePixels(ev.region());
}

void Editor::onSpritePixelRatioChanged(doc::DocumentEvent& ev)
{
  m_proj.setPixelRatio(ev.sprite()->pixelRatio());
  invalidate();
}

void Editor::onAddFrameTag(DocumentEvent& ev)
{
  m_tagFocusBand = -1;
}

void Editor::onRemoveFrameTag(DocumentEvent& ev)
{
  m_tagFocusBand = -1;
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
          m_layer->isVisibleHierarchy() &&
          m_layer->isEditableHierarchy() &&
          !m_layer->isReference());
}

bool Editor::isInsideSelection()
{
  gfx::Point spritePos = screenToEditor(ui::get_mouse_position());
  KeyAction action = m_customizationDelegate->getPressedKeyAction(KeyContext::SelectionTool);
  return
    (action == KeyAction::None) &&
    m_document &&
    m_document->isMaskVisible() &&
    m_document->mask()->containsPoint(spritePos.x, spritePos.y);
}

EditorHit Editor::calcHit(const gfx::Point& mouseScreenPos)
{
  tools::Ink* ink = getCurrentEditorInk();

  if (ink) {
    // Check if we can transform slices
    if (ink->isSlice()) {
      if (m_docPref.show.slices()) {
        for (auto slice : m_sprite->slices()) {
          auto key = slice->getByFrame(m_frame);
          if (key) {
            gfx::Rect bounds = editorToScreen(key->bounds());
            gfx::Rect center = key->center();

            // Move bounds
            if (bounds.contains(mouseScreenPos) &&
                !bounds.shrink(5*guiscale()).contains(mouseScreenPos)) {
              int border =
                (mouseScreenPos.x <= bounds.x ? LEFT: 0) |
                (mouseScreenPos.y <= bounds.y ? TOP: 0) |
                (mouseScreenPos.x >= bounds.x2() ? RIGHT: 0) |
                (mouseScreenPos.y >= bounds.y2() ? BOTTOM: 0);

              EditorHit hit(EditorHit::SliceBounds);
              hit.setBorder(border);
              hit.setSlice(slice);
              return hit;
            }

            // Move center
            if (!center.isEmpty()) {
              center = editorToScreen(
                center.offset(key->bounds().origin()));

              bool horz1 = gfx::Rect(bounds.x, center.y-2*guiscale(), bounds.w, 5*guiscale()).contains(mouseScreenPos);
              bool horz2 = gfx::Rect(bounds.x, center.y2()-2*guiscale(), bounds.w, 5*guiscale()).contains(mouseScreenPos);
              bool vert1 = gfx::Rect(center.x-2*guiscale(), bounds.y, 5*guiscale(), bounds.h).contains(mouseScreenPos);
              bool vert2 = gfx::Rect(center.x2()-2*guiscale(), bounds.y, 5*guiscale(), bounds.h).contains(mouseScreenPos);

              if (horz1 || horz2 || vert1 || vert2) {
                int border =
                  (horz1 ? TOP: 0) |
                  (horz2 ? BOTTOM: 0) |
                  (vert1 ? LEFT: 0) |
                  (vert2 ? RIGHT: 0);
                EditorHit hit(EditorHit::SliceCenter);
                hit.setBorder(border);
                hit.setSlice(slice);
              return hit;
              }
            }

            // Move all the slice
            if (bounds.contains(mouseScreenPos)) {
              EditorHit hit(EditorHit::SliceBounds);
              hit.setBorder(CENTER | MIDDLE);
              hit.setSlice(slice);
              return hit;
            }
          }
        }
      }
    }
  }

  return EditorHit(EditorHit::None);
}

void Editor::setZoomAndCenterInMouse(const Zoom& zoom,
                                     const gfx::Point& mousePos,
                                     ZoomBehavior zoomBehavior)
{
  HideBrushPreview hide(m_brushPreview);
  View* view = View::getView(this);
  Rect vp = view->viewportBounds();
  Projection proj = m_proj;
  proj.setZoom(zoom);

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

  if (zoomBehavior == ZoomBehavior::MOUSE) {
    gfx::Point screenPos2 = editorToScreen(spritePos);

    if (m_proj.scaleX() > 1.0) {
      subpixelPos.x = (0.5 + screenPos.x - screenPos2.x) / m_proj.scaleX();
      if (proj.scaleX() > m_proj.scaleX()) {
        double t = 1.0 / proj.scaleX();
        if (subpixelPos.x >= 0.5-t && subpixelPos.x <= 0.5+t)
          subpixelPos.x = 0.5;
      }
    }

    if (m_proj.scaleY() > 1.0) {
      subpixelPos.y = (0.5 + screenPos.y - screenPos2.y) / m_proj.scaleY();
      if (proj.scaleY() > m_proj.scaleY()) {
        double t = 1.0 / proj.scaleY();
        if (subpixelPos.y >= 0.5-t && subpixelPos.y <= 0.5+t)
          subpixelPos.y = 0.5;
      }
    }
  }

  gfx::Point padding = calcExtraPadding(proj);
  gfx::Point scrollPos(
    padding.x - (screenPos.x-vp.x) + proj.applyX(spritePos.x+proj.removeX(1)/2) + int(proj.applyX(subpixelPos.x)),
    padding.y - (screenPos.y-vp.y) + proj.applyY(spritePos.y+proj.removeY(1)/2) + int(proj.applyY(subpixelPos.y)));

  setZoom(zoom);

  if ((m_proj.zoom() != zoom) || (screenPos != view->viewScroll())) {
    updateEditor();
    setEditorScroll(scrollPos);
  }

  flushRedraw();
}

void Editor::pasteImage(const Image* image, const Mask* mask)
{
  ASSERT(image);

  base::UniquePtr<Mask> temp_mask;
  if (!mask) {
    gfx::Rect visibleBounds = getVisibleSpriteBounds();
    gfx::Rect imageBounds = image->bounds();

    temp_mask.reset(new Mask);
    temp_mask->replace(
      gfx::Rect(visibleBounds.x + visibleBounds.w/2 - imageBounds.w/2,
                visibleBounds.y + visibleBounds.h/2 - imageBounds.h/2,
                imageBounds.w, imageBounds.h));

    mask = temp_mask.get();
  }

  // Change to a selection tool: it's necessary for PixelsMovement
  // which will use the extra cel for transformation preview, and is
  // not compatible with the drawing cursor preview which overwrite
  // the extra cel.
  if (!getCurrentEditorInk()->isSelection()) {
    tools::Tool* defaultSelectionTool =
      App::instance()->toolBox()->getToolById(tools::WellKnownTools::RectangularMarquee);

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
    if (!visibleBounds.contains(mask->bounds().center())) {
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

void Editor::notifyZoomChanged()
{
  m_observers.notifyZoomChanged(this);
}

bool Editor::checkForScroll(ui::MouseMessage* msg)
{
  tools::Ink* clickedInk = getCurrentEditorInk();

  // Start scroll loop
  if (msg->middle() || clickedInk->isScrollMovement()) { // TODO msg->middle() should be customizable
    startScrollingState(msg);
    return true;
  }
  else
    return false;
}

bool Editor::checkForZoom(ui::MouseMessage* msg)
{
  tools::Ink* clickedInk = getCurrentEditorInk();

  // Start scroll loop
  if (clickedInk->isZoom()) {
    startZoomingState(msg);
    return true;
  }
  else
    return false;
}

void Editor::startScrollingState(ui::MouseMessage* msg)
{
  EditorStatePtr newState(new ScrollingState);
  setState(newState);
  newState->onMouseDown(this, msg);
}

void Editor::startZoomingState(ui::MouseMessage* msg)
{
  EditorStatePtr newState(new ZoomingState);
  setState(newState);
  newState->onMouseDown(this, msg);
}

void Editor::play(const bool playOnce,
                  const bool playAll)
{
  ASSERT(m_state);
  if (!m_state)
    return;

  if (m_isPlaying)
    stop();

  m_isPlaying = true;
  setState(EditorStatePtr(new PlayState(playOnce, playAll)));
}

void Editor::stop()
{
  ASSERT(m_state);
  if (!m_state)
    return;

  if (m_isPlaying) {
    while (m_state && !dynamic_cast<PlayState*>(m_state.get()))
      backToPreviousState();

    m_isPlaying = false;

    ASSERT(m_state && dynamic_cast<PlayState*>(m_state.get()));
    if (m_state)
      backToPreviousState();
  }
}

bool Editor::isPlaying() const
{
  return m_isPlaying;
}

void Editor::showAnimationSpeedMultiplierPopup(Option<bool>& playOnce,
                                               Option<bool>& playAll,
                                               const bool withStopBehaviorOptions)
{
  const double options[] = { 0.25, 0.5, 1.0, 1.5, 2.0, 3.0 };
  Menu menu;

  for (double option : options) {
    MenuItem* item = new MenuItem("Speed x" + base::convert_to<std::string>(option));
    item->Click.connect(base::Bind<void>(&Editor::setAnimationSpeedMultiplier, this, option));
    item->setSelected(m_aniSpeed == option);
    menu.addChild(item);
  }

  menu.addChild(new MenuSeparator);

  // Play once option
  {
    MenuItem* item = new MenuItem("Play Once");
    item->Click.connect(
      [&playOnce]() {
        playOnce(!playOnce());
      });
    item->setSelected(playOnce());
    menu.addChild(item);
  }

  // Play all option
  {
    MenuItem* item = new MenuItem("Play All Frames (Ignore Tags)");
    item->Click.connect(
      [&playAll]() {
        playAll(!playAll());
      });
    item->setSelected(playAll());
    menu.addChild(item);
  }

  if (withStopBehaviorOptions) {
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

  if (isPlaying()) {
    // Re-play
    stop();
    play(playOnce(),
         playAll());
  }
}

double Editor::getAnimationSpeedMultiplier() const
{
  return m_aniSpeed;
}

void Editor::setAnimationSpeedMultiplier(double speed)
{
  m_aniSpeed = speed;
}

void Editor::showMouseCursor(CursorType cursorType,
                             const Cursor* cursor)
{
  m_brushPreview.hide();
  ui::set_mouse_cursor(cursorType, cursor);
}

void Editor::showBrushPreview(const gfx::Point& screenPos)
{
  if (Preferences::instance().cursor.paintingCursorType() !=
      app::gen::PaintingCursorType::SIMPLE_CROSSHAIR)
    ui::set_mouse_cursor(kNoCursor);

  m_brushPreview.show(screenPos);
}

// static
ImageBufferPtr Editor::getRenderImageBuffer()
{
  return m_renderBuffer;
}

// static
gfx::Point Editor::calcExtraPadding(const Projection& proj)
{
  View* view = View::getView(this);
  if (view) {
    Rect vp = view->viewportBounds();
    return gfx::Point(
      std::max<int>(vp.w/2, vp.w - proj.applyX(m_sprite->width())),
      std::max<int>(vp.h/2, vp.h - proj.applyY(m_sprite->height())));
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

void Editor::invalidateIfActive()
{
  if (isActive())
    invalidate();
}

bool Editor::showAutoCelGuides()
{
  return
    (getCurrentEditorInk()->isCelMovement() &&
     m_docPref.show.autoGuides() &&
     m_customizationDelegate &&
     int(m_customizationDelegate->getPressedKeyAction(KeyContext::MoveTool) & KeyAction::AutoSelectLayer));
}

void Editor::updateAutoCelGuides(ui::Message* msg)
{
  Cel* oldShowGuidesThisCel = m_showGuidesThisCel;

  // Check if the user is pressing the Ctrl or Cmd key on move
  // tool to show automatic guides.
  if (showAutoCelGuides() &&
      m_state->requireBrushPreview()) {
    ui::MouseMessage* mouseMsg = dynamic_cast<ui::MouseMessage*>(msg);

    ColorPicker picker;
    picker.pickColor(getSite(),
                     screenToEditorF(mouseMsg ? mouseMsg->position():
                                                ui::get_mouse_position()),
                     m_proj, ColorPicker::FromComposition);
    m_showGuidesThisCel = (picker.layer() ? picker.layer()->cel(m_frame):
                                            nullptr);
  }
  else {
    m_showGuidesThisCel = nullptr;
  }

  if (m_showGuidesThisCel != oldShowGuidesThisCel)
    invalidate();
}

} // namespace app
