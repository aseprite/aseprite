// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/editor.h"

#include "app/app.h"
#include "app/app_menus.h"
#include "app/color.h"
#include "app/color_picker.h"
#include "app/color_utils.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/commands/quick_command.h"
#include "app/console.h"
#include "app/doc_event.h"
#include "app/i18n/strings.h"
#include "app/ini_file.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/pref/preferences.h"
#include "app/snap_to_grid.h"
#include "app/tools/active_tool.h"
#include "app/tools/controller.h"
#include "app/tools/ink.h"
#include "app/tools/tool.h"
#include "app/tools/tool_box.h"
#include "app/ui/color_bar.h"
#include "app/ui/context_bar.h"
#include "app/ui/doc_view.h"
#include "app/ui/editor/drawing_state.h"
#include "app/ui/editor/editor_customization_delegate.h"
#include "app/ui/editor/editor_decorator.h"
#include "app/ui/editor/editor_render.h"
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
#include "app/ui/timeline/timeline.h"
#include "app/ui/toolbar.h"
#include "app/ui_context.h"
#include "app/util/layer_utils.h"
#include "base/chrono.h"
#include "base/convert_to.h"
#include "doc/doc.h"
#include "doc/mask_boundaries.h"
#include "doc/slice.h"
#include "fmt/format.h"
#include "os/color_space.h"
#include "os/sampling.h"
#include "os/surface.h"
#include "os/system.h"
#include "render/rasterize.h"
#include "ui/ui.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <memory>

namespace app {

using namespace app::skin;
using namespace gfx;
using namespace ui;
using namespace render;

// TODO these should be grouped in some kind of "performance counters"
static base::Chrono renderChrono;
static double renderElapsed = 0.0;

class EditorPostRenderImpl : public EditorPostRender {
public:
  EditorPostRenderImpl(Editor* editor, Graphics* g)
    : m_editor(editor)
    , m_g(g) {
  }

  Editor* getEditor() override {
    return m_editor;
  }

  Graphics* getGraphics() override {
    return m_g;
  }

  void drawLine(gfx::Color color, int x1, int y1, int x2, int y2) override {
    gfx::Point a(x1, y1);
    gfx::Point b(x2, y2);
    a = m_editor->editorToScreen(a);
    b = m_editor->editorToScreen(b);
    gfx::Rect bounds = m_editor->bounds();
    a.x -= bounds.x;
    a.y -= bounds.y;
    b.x -= bounds.x;
    b.y -= bounds.y;
    m_g->drawLine(color, a, b);
  }

  void drawRect(gfx::Color color, const gfx::Rect& rc) override {
    gfx::Rect rc2 = m_editor->editorToScreen(rc);
    gfx::Rect bounds = m_editor->bounds();
    rc2.x -= bounds.x;
    rc2.y -= bounds.y;
    m_g->drawRect(color, rc2);
  }

  void fillRect(gfx::Color color, const gfx::Rect& rc) override {
    gfx::Rect rc2 = m_editor->editorToScreen(rc);
    gfx::Rect bounds = m_editor->bounds();
    rc2.x -= bounds.x;
    rc2.y -= bounds.y;
    m_g->fillRect(color, rc2);
  }

private:
  Editor* m_editor;
  Graphics* m_g;
};

// static
Editor* Editor::m_activeEditor = nullptr;

// static
std::unique_ptr<EditorRender> Editor::m_renderEngine = nullptr;

Editor::Editor(Doc* document, EditorFlags flags, EditorStatePtr state)
  : Widget(Editor::Type())
  , m_state(state == nullptr ? std::make_shared<StandbyState>(): state)
  , m_decorator(NULL)
  , m_document(document)
  , m_sprite(m_document->sprite())
  , m_layer(m_sprite->root()->firstLayer())
  , m_frame(frame_t(0))
  , m_docPref(Preferences::instance().document(document))
  , m_tiledModeHelper(app::TiledModeHelper(m_docPref.tiled.mode(), m_sprite))
  , m_brushPreview(this)
  , m_toolLoopModifiers(tools::ToolLoopModifiers::kNone)
  , m_padding(0, 0)
  , m_antsTimer(100, this)
  , m_antsOffset(0)
  , m_customizationDelegate(NULL)
  , m_docView(NULL)
  , m_flags(flags)
  , m_secondaryButton(false)
  , m_flashing(Flashing::None)
  , m_aniSpeed(1.0)
  , m_isPlaying(false)
  , m_showGuidesThisCel(nullptr)
  , m_showAutoCelGuides(false)
  , m_tagFocusBand(-1)
{
  if (!m_renderEngine)
    m_renderEngine = std::make_unique<EditorRender>();

  m_proj.setPixelRatio(m_sprite->pixelRatio());

  // Add the first state into the history.
  m_statesHistory.push(m_state);

  this->setFocusStop(true);

  App::instance()->activeToolManager()->add_observer(this);

  m_fgColorChangeConn =
    Preferences::instance().colorBar.fgColor.AfterChange.connect(
      [this]{ onFgColorChange(); });

  m_samplingChangeConn =
    Preferences::instance().editor.downsampling.AfterChange.connect(
      [this]{ onSamplingChange(); });

  m_contextBarBrushChangeConn =
    App::instance()->contextBar()->BrushChange.connect(
      [this]{ onContextBarBrushChange(); });

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

  m_tiledConnBefore = m_docPref.tiled.BeforeChange.connect([this]{ onTiledModeBeforeChange(); });
  m_tiledConn = m_docPref.tiled.AfterChange.connect([this]{ onTiledModeChange(); });
  m_gridConn = m_docPref.grid.AfterChange.connect([this]{ invalidate(); });
  m_pixelGridConn = m_docPref.pixelGrid.AfterChange.connect([this]{ invalidate(); });
  m_bgConn = m_docPref.bg.AfterChange.connect([this]{ invalidate(); });
  m_onionskinConn = m_docPref.onionskin.AfterChange.connect([this]{ invalidate(); });
  m_symmetryModeConn = Preferences::instance().symmetryMode.enabled.AfterChange.connect([this]{ invalidateIfActive(); });
  m_showExtrasConn =
    m_docPref.show.AfterChange.connect(
      [this]{ onShowExtrasChange(); });

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
  BrushPreview::destroyInternals();
  if (m_renderEngine)
    m_renderEngine.reset();
}

bool Editor::isUsingNewRenderEngine() const
{
  ASSERT(m_sprite);
  return
    // TODO add an option to the ShaderRenderer to work as the "old"
    //      engine (screen pixel by screen pixel) or as the "new"
    //      engine (sprite pixel by sprite pixel)
    (m_renderEngine->type() == EditorRender::Type::kShaderRenderer)
    ||
    (Preferences::instance().experimental.newRenderEngine()
     // Reference layers + zoom > 100% need the old render engine for
     // sub-pixel rendering.
     && (!m_sprite->hasVisibleReferenceLayers()
         || (m_proj.scaleX() <= 1.0
             && m_proj.scaleY() <= 1.0)));
}

// static
WidgetType Editor::Type()
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
  setCursor(mousePosInDisplay());

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

#if ENABLE_DEVMODE
  // TODO put this in other widget
  if (Preferences::instance().perf.showRenderTime()) {
    if (!m_perfInfoBounds.isEmpty())
      region |= gfx::Region(m_perfInfoBounds);
  }
#endif // ENABLE_DEVMODE
}

void Editor::setLayer(const Layer* layer)
{
  const bool changed = (m_layer != layer);
  const bool gridVisible = (changed && m_docPref.show.grid());

  doc::Grid oldGrid, newGrid;
  if (gridVisible)
    oldGrid = getSite().grid();

  m_observers.notifyBeforeLayerChanged(this);

  // Remove extra cel information if we change between different layer
  // type (e.g. from a tilemap layer to an image layer). This is
  // useful to avoid a flickering effect in the preview window (using
  // a non-updated extra cel to patch the new "layer" with the
  // background of the previous selected "m_layer".
  if ((layer == nullptr) ||
      (m_layer != nullptr && m_layer->type() != layer->type())) {
    m_document->setExtraCel(ExtraCelRef(nullptr));
  }

  m_layer = const_cast<Layer*>(layer);
  m_observers.notifyAfterLayerChanged(this);

  if (gridVisible)
    newGrid = getSite().grid();

  if (m_document && changed) {
    if (// If the onion skinning depends on the active layer
        m_docPref.onionskin.currentLayer() ||
        // If the user want to see the active layer edges...
        m_docPref.show.layerEdges() ||
        // If there is a different opacity for nonactive-layers
        Preferences::instance().experimental.nonactiveLayersOpacity() < 255 ||
        // If the automatic cel guides are visible...
        m_showGuidesThisCel ||
        // If grid settings changed
        (gridVisible &&
         (oldGrid.tileSize() != newGrid.tileSize() ||
          oldGrid.origin() != newGrid.origin()))) {
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

  // Invalidate canvas area
  invalidateCanvas();
  updateStatusBar();
}

void Editor::getSite(Site* site) const
{
  site->document(m_document);
  site->sprite(m_sprite);
  site->layer(m_layer);
  site->frame(m_frame);

  if (!m_selectedSlices.empty() &&
      getCurrentEditorInk()->isSlice()) {
    site->selectedSlices(m_selectedSlices);
  }

  // TODO we should not access timeline directly here
  Timeline* timeline = App::instance()->timeline();
  if (timeline &&
      timeline->isVisible() &&
      timeline->range().enabled()) {
    site->range(timeline->range());
  }

  if (m_layer && m_layer->isTilemap()) {
    TilemapMode tilemapMode = site->tilemapMode();
    TilesetMode tilesetMode = site->tilesetMode();
    const ColorBar* colorbar = ColorBar::instance();
    ASSERT(colorbar);
    if (colorbar) {
      tilemapMode = colorbar->tilemapMode();
      tilesetMode = colorbar->tilesetMode();
    }
    site->tilemapMode(tilemapMode);
    site->tilesetMode(tilesetMode);
  }
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

    if (isActive())
      App::instance()->contextBar()->updateSamplingVisibility();
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
  if (Preferences::instance().editor.autoFit())
    setScrollAndZoomToFitScreen();
  else
    setScrollToCenter();
}

void Editor::setScrollToCenter()
{
  View* view = View::getView(this);
  Rect vp = view->viewportBounds();
  gfx::Size canvas = canvasSize();

  setEditorScroll(
    gfx::Point(
      m_padding.x - vp.w/2 + m_proj.applyX(canvas.w)/2,
      m_padding.y - vp.h/2 + m_proj.applyY(canvas.h)/2));
}

void Editor::setScrollAndZoomToFitScreen()
{
  View* view = View::getView(this);
  gfx::Rect vp = view->viewportBounds();
  gfx::Size canvas = canvasSize();
  Zoom zoom = m_proj.zoom();

  if (float(vp.w) / float(canvas.w) <
      float(vp.h) / float(canvas.h)) {
    if (vp.w < m_proj.applyX(canvas.w)) {
      while (vp.w < m_proj.applyX(canvas.w)) {
        if (!zoom.out())
          break;
        m_proj.setZoom(zoom);
      }
    }
    else if (vp.w > m_proj.applyX(canvas.w)) {
      bool out = true;
      while (vp.w > m_proj.applyX(canvas.w)) {
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
    if (vp.h < m_proj.applyY(canvas.h)) {
      while (vp.h < m_proj.applyY(canvas.h)) {
        if (!zoom.out())
          break;
        m_proj.setZoom(zoom);
      }
    }
    else if (vp.h > m_proj.applyY(canvas.h)) {
      bool out = true;
      while (vp.h > m_proj.applyY(canvas.h)) {
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

  updateEditor(false);
  setEditorScroll(
    gfx::Point(
      m_padding.x - vp.w/2 + m_proj.applyX(canvas.w)/2,
      m_padding.y - vp.h/2 + m_proj.applyY(canvas.h)/2));
}

// Sets the scroll position of the editor
void Editor::setEditorScroll(const gfx::Point& scroll)
{
  View::getView(this)->setViewScroll(scroll);
}

void Editor::setEditorZoom(const render::Zoom& zoom)
{
  setZoomAndCenterInMouse(
    zoom, mousePosInDisplay(),
    Editor::ZoomBehavior::CENTER);
}

void Editor::updateEditor(const bool restoreScrollPos)
{
  View::getView(this)->updateView(restoreScrollPos);
}

void Editor::drawOneSpriteUnclippedRect(ui::Graphics* g, const gfx::Rect& spriteRectToDraw, int dx, int dy)
{
  // Clip from sprite and apply zoom
  gfx::Rect rc = m_sprite->bounds().createIntersection(spriteRectToDraw);
  rc = m_proj.apply(rc);

  gfx::Rect dest(dx + m_padding.x + rc.x,
                 dy + m_padding.y + rc.y, 0, 0);

  // Clip from graphics/screen
  const gfx::Rect& clip = g->getClipBounds();
  if (dest.x < clip.x) {
    rc.x += clip.x - dest.x;
    rc.w -= clip.x - dest.x;
    dest.x = clip.x;
  }
  if (dest.y < clip.y) {
    rc.y += clip.y - dest.y;
    rc.h -= clip.y - dest.y;
    dest.y = clip.y;
  }
  if (dest.x+rc.w > clip.x+clip.w) {
    rc.w = clip.x+clip.w-dest.x;
  }
  if (dest.y+rc.h > clip.y+clip.h) {
    rc.h = clip.y+clip.h-dest.y;
  }

  if (rc.isEmpty())
    return;

  // Bounds of pixels from the sprite canvas that will be exposed in
  // this render cycle.
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

  expose &= m_sprite->bounds();

  const int maxw = std::max(0, m_sprite->width()-expose.x);
  const int maxh = std::max(0, m_sprite->height()-expose.y);
  expose.w = std::clamp(expose.w, 0, maxw);
  expose.h = std::clamp(expose.h, 0, maxh);
  if (expose.isEmpty())
    return;

  // rc2 is the rectangle used to create a temporal rendered image of the sprite
  const auto& pref = Preferences::instance();
  const bool newEngine = isUsingNewRenderEngine();
  gfx::Rect rc2;
  if (newEngine) {
    rc2 = expose;               // New engine, exposed rectangle (without zoom)
    dest.x = dx + m_padding.x + m_proj.applyX(rc2.x);
    dest.y = dy + m_padding.y + m_proj.applyY(rc2.y);
    dest.w = m_proj.applyX(rc2.w);
    dest.h = m_proj.applyY(rc2.h);
  }
  else {
    rc2 = rc;                   // Old engine, same rectangle with zoom
    dest.w = rc.w;
    dest.h = rc.h;
  }

  // Convert the render to a os::Surface
  static os::SurfaceRef rendered = nullptr; // TODO move this to other centralized place
  const auto& renderProperties = m_renderEngine->properties();
  try {
    // Generate a "expose sprite pixels" notification. This is used by
    // tool managers that need to validate this region (copy pixels from
    // the original cel) before it can be used by the RenderEngine.
    m_document->notifyExposeSpritePixels(m_sprite, gfx::Region(expose));

    m_renderEngine->setNewBlendMethod(pref.experimental.newBlend());
    m_renderEngine->setRefLayersVisiblity(true);
    m_renderEngine->setSelectedLayer(m_layer);
    if (m_flags & Editor::kUseNonactiveLayersOpacityWhenEnabled)
      m_renderEngine->setNonactiveLayersOpacity(pref.experimental.nonactiveLayersOpacity());
    else
      m_renderEngine->setNonactiveLayersOpacity(255);
    m_renderEngine->setupBackground(m_document, IMAGE_RGB);
    m_renderEngine->disableOnionskin();

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

        Tag* tag = nullptr;
        if (m_docPref.onionskin.loopTag())
          tag = m_sprite->tags().innerTag(m_frame);
        opts.loopTag(tag);

        m_renderEngine->setOnionskin(opts);
      }
    }

    ExtraCelRef extraCel = m_document->extraCel();
    if (extraCel &&
        extraCel->type() != render::ExtraType::NONE) {
      m_renderEngine->setExtraImage(
        extraCel->type(),
        extraCel->cel(),
        extraCel->image(),
        extraCel->blendMode(),
        m_layer, m_frame);
    }

    // Render background first (e.g. new ShaderRenderer will paint the
    // background on the screen first and then composite the rendered
    // sprite on it.)
    if (renderProperties.renderBgOnScreen) {
      m_renderEngine->setProjection(m_proj);
      m_renderEngine->renderCheckeredBackground(
        g->getInternalSurface(),
        m_sprite,
        gfx::Clip(dest.x + g->getInternalDeltaX(),
                  dest.y + g->getInternalDeltaY(),
                  m_proj.apply(rc2)));
    }

    // Create a temporary surface to draw the sprite on it
    if (!rendered ||
        rendered->width() < rc2.w ||
        rendered->height() < rc2.h ||
        rendered->colorSpace() != m_document->osColorSpace()) {
      const int maxw = std::max(rc2.w, rendered ? rendered->width(): 0);
      const int maxh = std::max(rc2.h, rendered ? rendered->height(): 0);
      rendered = os::instance()->makeRgbaSurface(
        maxw, maxh, m_document->osColorSpace());
    }

    m_renderEngine->setProjection(
      newEngine ? render::Projection(): m_proj);
    m_renderEngine->renderSprite(
      rendered.get(), m_sprite, m_frame, gfx::Clip(0, 0, rc2));

    m_renderEngine->removeExtraImage();

    // If the checkered background is visible in this sprite, we save
    // all settings of the background for this document.
    if (!m_sprite->isOpaque())
      m_docPref.bg.forceSection();
  }
  catch (const std::exception& e) {
    Console::showException(e);
  }

  if (rendered && rendered->nativeHandle()) {
    if (newEngine) {
      os::Sampling sampling;
      if (m_proj.scaleX() < 1.0) {
        switch (pref.editor.downsampling()) {
          case gen::Downsampling::NEAREST:
            sampling = os::Sampling(os::Sampling::Filter::Nearest);
            break;
          case gen::Downsampling::BILINEAR:
            sampling = os::Sampling(os::Sampling::Filter::Linear);
            break;
          case gen::Downsampling::BILINEAR_MIPMAP:
            sampling = os::Sampling(os::Sampling::Filter::Linear,
                                    os::Sampling::Mipmap::Nearest);
            break;
          case gen::Downsampling::TRILINEAR_MIPMAP:
            sampling = os::Sampling(os::Sampling::Filter::Linear,
                                    os::Sampling::Mipmap::Linear);
            break;
        }
      }

      os::Paint p;
      if (renderProperties.requiresRgbaBackbuffer)
        p.blendMode(os::BlendMode::SrcOver);
      else
        p.blendMode(os::BlendMode::Src);

      g->drawSurface(rendered.get(),
                     gfx::Rect(0, 0, rc2.w, rc2.h),
                     dest,
                     sampling,
                     &p);
    }
    else {
      g->blit(rendered.get(), 0, 0, dest.x, dest.y, dest.w, dest.h);
    }
  }

  // Draw grids
  {
    gfx::Rect enclosingRect(
      m_padding.x + dx,
      m_padding.y + dy,
      m_proj.applyX(m_sprite->width()),
      m_proj.applyY(m_sprite->height()));

    IntersectClip clip(g, dest);
    if (clip) {
      // Draw the pixel grid
      if ((m_proj.zoom().scale() > 2.0) && m_docPref.show.pixelGrid()) {
        int alpha = m_docPref.pixelGrid.opacity();

        if (m_docPref.pixelGrid.autoOpacity()) {
          alpha = int(alpha * (m_proj.zoom().scale()-2.) / (16.-2.));
          alpha = std::clamp(alpha, 0, 255);
        }

        drawGrid(g, enclosingRect, Rect(0, 0, 1, 1),
                 m_docPref.pixelGrid.color(), alpha);

        // Save all pixel grid settings that are unset
        m_docPref.pixelGrid.forceSection();
      }
      m_docPref.show.pixelGrid.forceDirtyFlag();

      // Draw the grid
      if (m_docPref.show.grid()) {
        gfx::Rect gridrc;
        if (!m_state->getGridBounds(this, gridrc))
          gridrc = getSite().gridBounds();

        if (m_proj.applyX(gridrc.w) > 2 &&
            m_proj.applyY(gridrc.h) > 2) {
          int alpha = m_docPref.grid.opacity();

          if (m_docPref.grid.autoOpacity()) {
            double len = (m_proj.applyX(gridrc.w) +
                          m_proj.applyY(gridrc.h)) / 2.;
            alpha = int(alpha * len / 32.);
            alpha = std::clamp(alpha, 0, 255);
          }

          if (alpha > 8) {
            drawGrid(g, enclosingRect, gridrc,
                     m_docPref.grid.color(), alpha);
          }
        }

        // Save all grid settings that are unset
        m_docPref.grid.forceSection();
      }
      m_docPref.show.grid.forceDirtyFlag();
    }
  }
}

void Editor::drawBackground(ui::Graphics* g)
{
  if (!(m_flags & kShowOutside))
    return;

  auto theme = SkinTheme::get(this);

  gfx::Size canvas = canvasSize();
  gfx::Rect rc(0, 0, canvas.w, canvas.h);
  rc = editorToScreen(rc);
  rc.offset(-bounds().origin());

  // Fill the outside (parts of the editor that aren't covered by the
  // sprite).
  gfx::Region outside(clientBounds());
  outside.createSubtraction(outside, gfx::Region(rc));
  g->fillRegion(theme->colors.editorFace(), outside);

  // Draw the borders that enclose the sprite.
  rc.enlarge(1);
  g->drawRect(theme->colors.editorSpriteBorder(), rc);
  g->drawHLine(theme->colors.editorSpriteBottomBorder(), rc.x, rc.y2(), rc.w);
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

  // Document preferences
  if (int(m_docPref.tiled.mode()) & int(filters::TiledMode::X_AXIS)) {
    drawOneSpriteUnclippedRect(g, rc, spriteRect.w, 0);
    drawOneSpriteUnclippedRect(g, rc, spriteRect.w*2, 0);

    enclosingRect = gfx::Rect(spriteRect.x, spriteRect.y, spriteRect.w*3, spriteRect.h);
  }

  if (int(m_docPref.tiled.mode()) & int(filters::TiledMode::Y_AXIS)) {
    drawOneSpriteUnclippedRect(g, rc, 0, spriteRect.h);
    drawOneSpriteUnclippedRect(g, rc, 0, spriteRect.h*2);

    enclosingRect = gfx::Rect(spriteRect.x, spriteRect.y, spriteRect.w, spriteRect.h*3);
  }

  if (m_docPref.tiled.mode() == filters::TiledMode::BOTH) {
    drawOneSpriteUnclippedRect(g, rc, spriteRect.w,   spriteRect.h);
    drawOneSpriteUnclippedRect(g, rc, spriteRect.w*2, spriteRect.h);
    drawOneSpriteUnclippedRect(g, rc, spriteRect.w,   spriteRect.h*2);
    drawOneSpriteUnclippedRect(g, rc, spriteRect.w*2, spriteRect.h*2);

    enclosingRect = gfx::Rect(
      spriteRect.x, spriteRect.y,
      spriteRect.w*3, spriteRect.h*3);
  }

  // Draw slices
  if (m_docPref.show.slices())
    drawSlices(g);

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
                     spriteRect.x + m_proj.applyX(mainTilePosition().x) + int(m_proj.applyX<double>(x)),
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
                     spriteRect.y + m_proj.applyY(mainTilePosition().y) + int(m_proj.applyY<double>(y)),
                     enclosingRect.w);
      }
    }
  }

  // Draw active layer/cel edges
  if ((m_docPref.show.layerEdges() || m_showAutoCelGuides) &&
      // Show layer edges and possibly cel guides only on states that
      // allows it (e.g scrolling state)
      m_state->allowLayerEdges()) {
    Cel* cel = (m_layer ? m_layer->cel(m_frame): nullptr);
    if (cel) {
      gfx::Color color = color_utils::color_for_ui(Preferences::instance().guides.layerEdgesColor());
      drawCelBounds(g, cel, color);

      // Draw tile numbers
      if (m_docPref.show.tileNumbers() &&
          cel->layer()->isTilemap()) {
        drawTileNumbers(g, cel);
      }

      // Draw auto-guides to other cel
      if (m_showAutoCelGuides &&
          m_showGuidesThisCel != cel) {
        drawCelGuides(g, cel, m_showGuidesThisCel);
      }
    }
  }

  // Draw the mask
  if (m_document->hasMaskBoundaries())
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

  ScreenGraphics screenGraphics(display());
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

  ASSERT(m_document->hasMaskBoundaries());

  gfx::Point pt = mainTilePosition();
  pt.x = m_padding.x + m_proj.applyX(pt.x);
  pt.y = m_padding.y + m_proj.applyY(pt.y);

  // Create the mask boundaries path
  auto& segs = m_document->maskBoundaries();
  segs.createPathIfNeeeded();

  ui::Paint paint;
  paint.style(ui::Paint::Stroke);
  set_checkered_paint_mode(paint, m_antsOffset,
                           gfx::rgba(0, 0, 0, 255),
                           gfx::rgba(255, 255, 255, 255));

  // We translate the path instead of applying a matrix to the
  // ui::Graphics so the "checkered" pattern is not scaled too.
  gfx::Path path;
  segs.path().transform(m_proj.scaleMatrix(), &path);
  path.offset(pt.x, pt.y);
  g->drawPath(path, paint);
}

void Editor::drawMaskSafe()
{
  if ((m_flags & kShowMask) == 0)
    return;

  if (isVisible() &&
      m_document &&
      m_document->hasMaskBoundaries()) {
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
  RectF gridF(grid);
  gridF = editorToScreenF(gridF);
  if (gridF.w < 1. || gridF.h < 1.)
    return;

  // Adjust for client area
  gfx::Rect bounds = this->bounds();
  gridF.offset(-bounds.origin());

  while (gridF.x-gridF.w >= spriteBounds.x) gridF.x -= gridF.w;
  while (gridF.y-gridF.h >= spriteBounds.y) gridF.y -= gridF.h;

  // Get the grid's color
  gfx::Color grid_color = color_utils::color_for_ui(color);
  grid_color = gfx::rgba(
    gfx::getr(grid_color),
    gfx::getg(grid_color),
    gfx::getb(grid_color), alpha);

  // Draw horizontal lines
  int x1 = spriteBounds.x;
  int y1 = gridF.y;
  int x2 = spriteBounds.x + spriteBounds.w;
  int y2 = spriteBounds.y + spriteBounds.h;

  for (double c=y1; c<=y2; c+=gridF.h)
    g->drawHLine(grid_color, x1, c, spriteBounds.w);

  // Draw vertical lines
  x1 = gridF.x;
  y1 = spriteBounds.y;

  for (double c=x1; c<=x2; c+=gridF.w)
    g->drawVLine(grid_color, c, y1, spriteBounds.h);
}

void Editor::drawSlices(ui::Graphics* g)
{
  if ((m_flags & kShowSlices) == 0)
    return;

  if (!isVisible() || !m_document)
    return;

  auto theme = SkinTheme::get(this);
  gfx::Point mainOffset(mainTilePosition());

  for (auto slice : m_sprite->slices()) {
    auto key = slice->getByFrame(m_frame);
    if (!key)
      continue;

    doc::color_t docColor = slice->userData().color();
    gfx::Color color = gfx::rgba(doc::rgba_getr(docColor),
                                 doc::rgba_getg(docColor),
                                 doc::rgba_getb(docColor),
                                 doc::rgba_geta(docColor));
    gfx::Rect out = key->bounds();
    out.offset(mainOffset);
    out = editorToScreen(out);
    out.offset(-bounds().origin());

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

    if (isSliceSelected(slice) &&
        getCurrentEditorInk()->isSlice()) {
      PaintWidgetPartInfo info;
      theme->paintWidgetPart(
        g, theme->styles.colorbarSelection(), out, info);
    }
    else {
      g->drawRect(color, out);
    }
  }
}

void Editor::drawTileNumbers(ui::Graphics* g, const Cel* cel)
{
  gfx::Color color = color_utils::color_for_ui(Preferences::instance().guides.autoGuidesColor());
  gfx::Color fgColor = color_utils::blackandwhite_neg(color);

  const doc::Grid grid = getSite().grid();
  const gfx::Size tileSize = editorToScreen(grid.tileToCanvas(gfx::Rect(0, 0, 1, 1))).size();
  if (tileSize.h > g->font()->height()) {
    const gfx::Point offset =
      gfx::Point(tileSize.w/2,
                 tileSize.h/2 - g->font()->height()/2)
      + mainTilePosition();

    int ti_offset =
      static_cast<LayerTilemap*>(cel->layer())->tileset()->baseIndex() - 1;

    const doc::Image* image = cel->image();
    std::string text;
    for (int y=0; y<image->height(); ++y) {
      for (int x=0; x<image->width(); ++x) {
        doc::tile_t t = image->getPixel(x, y);
        if (t != doc::notile) {
          gfx::Point pt = editorToScreen(grid.tileToCanvas(gfx::Point(x, y)));
          pt -= bounds().origin();
          pt += offset;

          text = fmt::format("{}", int(t & doc::tile_i_mask) + ti_offset);
          pt.x -= g->measureUIText(text).w/2;
          g->drawText(text, fgColor, color, pt);
        }
      }
    }
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

    const gfx::Color color = color_utils::color_for_ui(Preferences::instance().guides.autoGuidesColor());
    drawCelBounds(g, mouseCel, color);
  }
  // Use whole canvas
  else {
    sprCmpBounds = m_sprite->bounds();
    scrCmpBounds =
      editorToScreen(
        gfx::Rect(sprCmpBounds).offset(mainTilePosition()))
      .offset(gfx::Point(-bounds().origin()));
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
  g->drawHLine(color, std::min(scrX1, scrX2), scrY, std::abs(scrX2 - scrX1));

  // Vertical guide to touch the horizontal line
  {
    ui::Paint paint;
    ui::set_checkered_paint_mode(paint, 0, color, gfx::ColorNone);
    paint.color(color);

    if (scrY < scrCmpBounds.y)
      g->drawVLine(dottedX, scrCelBounds.y, scrCmpBounds.y - scrCelBounds.y, paint);
    else if (scrY > scrCmpBounds.y2())
      g->drawVLine(dottedX, scrCmpBounds.y2(), scrCelBounds.y2() - scrCmpBounds.y2(), paint);
  }

  auto text = fmt::format("{}px", ABS(sprX2 - sprX1));
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
  g->drawVLine(color, scrX, std::min(scrY1, scrY2), std::abs(scrY2 - scrY1));

  // Horizontal guide to touch the vertical line
  {
    ui::Paint paint;
    ui::set_checkered_paint_mode(paint, 0, color, gfx::ColorNone);
    paint.color(color);

    if (scrX < scrCmpBounds.x)
      g->drawHLine(scrCelBounds.x, dottedY, scrCmpBounds.x - scrCelBounds.x, paint);
    else if (scrX > scrCmpBounds.x2())
      g->drawHLine(scrCmpBounds.x2(), dottedY, scrCelBounds.x2() - scrCmpBounds.x2(), paint);
  }

  auto text = fmt::format("{}px", ABS(sprY2 - sprY1));
  g->drawText(text,
              color_utils::blackandwhite_neg(color), color,
              gfx::Point(scrX, (scrY1+scrY2)/2-textHeight()/2));
}

gfx::Rect Editor::getCelScreenBounds(const Cel* cel)
{
  gfx::Point mainOffset(mainTilePosition());
  gfx::Rect layerEdges;
  if (m_layer->isReference()) {
    layerEdges =
      editorToScreenF(
        gfx::RectF(cel->boundsF()).offset(mainOffset.x,
                                          mainOffset.y))
      .offset(gfx::PointF(-bounds().origin()));
  }
  else {
    layerEdges =
      editorToScreen(
        gfx::Rect(cel->bounds()).offset(mainOffset))
      .offset(-bounds().origin());
  }
  return layerEdges;
}

void Editor::flashCurrentLayer()
{
  if (!Preferences::instance().experimental.flashLayer())
    return;

  Site site = getSite();
  if (site.cel()) {
    // Hide and destroy the extra cel used by the brush preview
    // because we'll need to use the extra cel now for the flashing
    // layer.
    m_brushPreview.hide();

    m_renderEngine->removePreviewImage();

    ExtraCelRef extraCel(new ExtraCel);
    extraCel->setType(render::ExtraType::OVER_COMPOSITE);
    extraCel->setBlendMode(doc::BlendMode::NEG_BW);

    m_document->setExtraCel(extraCel);
    m_flashing = Flashing::WithFlashExtraCel;

    invalidateCanvas();
  }
}

gfx::Point Editor::autoScroll(const ui::MouseMessage* msg,
                              const AutoScroll dir)
{
  gfx::Point mousePos = msg->position();
  if (!Preferences::instance().editor.autoScroll())
    return mousePos;

  // Hide the brush preview
  //HideBrushPreview hide(m_brushPreview);
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

    mousePos -= delta;
    ui::set_mouse_position(mousePos,
                           display());

    m_oldPos = mousePos;
    mousePos = gfx::Point(
      std::clamp(mousePos.x, vp.x, vp.x2()-1),
      std::clamp(mousePos.y, vp.y, vp.y2()-1));
  }
  else
    m_oldPos = mousePos;

  return mousePos;
}

tools::Tool* Editor::getCurrentEditorTool() const
{
  return App::instance()->activeTool();
}

tools::Ink* Editor::getCurrentEditorInk() const
{
  tools::Ink* ink = m_state->getStateInk();
  if (ink)
    return ink;
  else
    return App::instance()->activeToolManager()->activeInk();
}

bool Editor::isAutoSelectLayer()
{
  tools::Ink* ink = getCurrentEditorInk();
  if (ink && ink->isAutoSelectLayer())
    return true;
  else
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

gfx::Point Editor::screenToEditorCeiling(const gfx::Point& pt)
{
  View* view = View::getView(this);
  Rect vp = view->viewportBounds();
  Point scroll = view->viewScroll();
  return gfx::Point(
    m_proj.removeXCeiling(pt.x - vp.x + scroll.x - m_padding.x),
    m_proj.removeYCeiling(pt.y - vp.y + scroll.y - m_padding.y));
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
    screenToEditorCeiling(rc.point2()));
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

Rect Editor::getViewportBounds()
{
  ui::View* view = View::getView(this);
  if (view)
    return screenToEditor(view->viewportBounds());
  else
    return bounds();
}

// Returns the visible area of the active sprite.
Rect Editor::getVisibleSpriteBounds()
{
  if (m_sprite)
    return getViewportBounds().createIntersection(gfx::Rect(canvasSize()));

  // This cannot happen, the sprite must be != nullptr. In old
  // Aseprite versions we were using one Editor to show multiple
  // sprites (switching the sprite inside the editor). Now we have one
  // (or more) editor(s) for each sprite.
  ASSERT(false);

  // Return an empty rectangle if there is not a active sprite.
  return Rect();
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

  updateEditor(false);
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
    auto atm = App::instance()->activeToolManager();
    tools::Tool* selectedTool = atm->selectedTool();

    // Don't change quicktools if we are in a selection tool and using
    // the selection modifiers.
    if (selectedTool->getInk(0)->isSelection() &&
        int(m_customizationDelegate->getPressedKeyAction(KeyContext::SelectionTool)) != 0) {
      if (atm->quickTool())
        atm->newQuickToolSelectedFromEditor(nullptr);
      return;
    }

    tools::Tool* newQuicktool =
      m_customizationDelegate->getQuickTool(selectedTool);

    // Check if the current state accept the given quicktool.
    if (newQuicktool && !m_state->acceptQuickTool(newQuicktool))
      return;

    atm->newQuickToolSelectedFromEditor(newQuicktool);
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

void Editor::updateToolLoopModifiersIndicators(const bool firstFromMouseDown)
{
  int modifiers = int(tools::ToolLoopModifiers::kNone);
  const bool autoSelectLayer = isAutoSelectLayer();
  bool newAutoSelectLayer = autoSelectLayer;
  KeyAction action;

  if (m_customizationDelegate) {
    auto atm = App::instance()->activeToolManager();

    // When the mouse is captured, is when we are scrolling, or
    // drawing, or moving, or selecting, etc. So several
    // parameters/tool-loop-modifiers are static.
    if (hasCapture()) {
      modifiers |= (int(m_toolLoopModifiers) &
                    (int(tools::ToolLoopModifiers::kReplaceSelection) |
                     int(tools::ToolLoopModifiers::kAddSelection) |
                     int(tools::ToolLoopModifiers::kSubtractSelection) |
                     int(tools::ToolLoopModifiers::kIntersectSelection)));

      tools::Tool* tool = atm->selectedTool();
      tools::Controller* controller = (tool ? tool->getController(0): nullptr);
      tools::Ink* ink = (tool ? tool->getInk(0): nullptr);

      // Shape tools modifiers (line, curves, rectangles, etc.)
      if (controller && controller->isTwoPoints()) {
        action = m_customizationDelegate->getPressedKeyAction(KeyContext::ShapeTool);

        // For two-points-selection-like tools (Rectangular/Elliptical
        // Marquee) we prefer to activate the
        // square-aspect/rotation/etc. only when the user presses the
        // modifier key again in the ToolLoop (and not before starting
        // the loop). So Alt+selection will add a selection, but
        // willn't start the square-aspect until we press Alt key
        // again, or Alt+Shift+selection tool will subtract the
        // selection but will not start the rotation until we release
        // and press the Alt key again.
        if (!firstFromMouseDown ||
            !ink || !ink->isSelection()) {
          if (int(action & KeyAction::MoveOrigin))
            modifiers |= int(tools::ToolLoopModifiers::kMoveOrigin);
          if (int(action & KeyAction::SquareAspect))
            modifiers |= int(tools::ToolLoopModifiers::kSquareAspect);
          if (int(action & KeyAction::DrawFromCenter))
            modifiers |= int(tools::ToolLoopModifiers::kFromCenter);
          if (int(action & KeyAction::RotateShape))
            modifiers |= int(tools::ToolLoopModifiers::kRotateShape);
        }
      }

      // Freehand modifiers
      if (controller && controller->isFreehand()) {
        action = m_customizationDelegate->getPressedKeyAction(KeyContext::FreehandTool);
        if (int(action & KeyAction::AngleSnapFromLastPoint))
          modifiers |= int(tools::ToolLoopModifiers::kSquareAspect);
      }
    }
    else {
      // We update the selection mode only if we're not selecting.
      action = m_customizationDelegate->getPressedKeyAction(KeyContext::SelectionTool);

      gen::SelectionMode mode = Preferences::instance().selection.mode();
      if (int(action & KeyAction::SubtractSelection) ||
          // Don't use "subtract" mode if the selection was activated
          // with the "right click mode = a selection-like tool"
          (m_secondaryButton &&
           atm->selectedTool() &&
           atm->selectedTool()->getInk(0)->isSelection())) {
        mode = gen::SelectionMode::SUBTRACT;
      }
      else if (int(action & KeyAction::IntersectSelection)) {
        mode = gen::SelectionMode::INTERSECT;
      }
      else if (int(action & KeyAction::AddSelection)) {
        mode = gen::SelectionMode::ADD;
      }
      switch (mode) {
        case gen::SelectionMode::DEFAULT:   modifiers |= int(tools::ToolLoopModifiers::kReplaceSelection);  break;
        case gen::SelectionMode::ADD:       modifiers |= int(tools::ToolLoopModifiers::kAddSelection);      break;
        case gen::SelectionMode::SUBTRACT:  modifiers |= int(tools::ToolLoopModifiers::kSubtractSelection); break;
        case gen::SelectionMode::INTERSECT: modifiers |= int(tools::ToolLoopModifiers::kIntersectSelection); break;
      }

      // For move tool
      action = m_customizationDelegate->getPressedKeyAction(KeyContext::MoveTool);
      if (int(action & KeyAction::AutoSelectLayer))
        newAutoSelectLayer = Preferences::instance().editor.autoSelectLayerQuick();
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
    site.tilemapMode(TilemapMode::Pixels);
    picker.pickColor(site, editorPos, m_proj,
                     ColorPicker::FromComposition);
    return picker.color();
  }
  else
    return app::Color::fromMask();
}

doc::tile_t Editor::getTileByPosition(const gfx::Point& mousePos)
{
  Site site = getSite();
  if (site.sprite()) {
    gfx::PointF editorPos = screenToEditorF(mousePos);

    ColorPicker picker;
    site.tilemapMode(TilemapMode::Tiles);
    picker.pickColor(site, editorPos, m_proj,
                     ColorPicker::FromComposition);

    return picker.tile();
  }
  else
    return doc::notile;
}

bool Editor::startStraightLineWithFreehandTool(const tools::Pointer* pointer)
{
  tools::Tool* tool = App::instance()->activeToolManager()->selectedTool();
  // TODO add support for more buttons (X1, X2, etc.)
  int i = (pointer && pointer->button() == tools::Pointer::Button::Right ? 1: 0);
  return
    (isActive() &&
     (hasMouse() || hasCapture()) &&
     tool &&
     tool->getController(i)->isFreehand() &&
     tool->getInk(i)->isPaint() &&
     (getCustomizationDelegate()
      ->getPressedKeyAction(KeyContext::FreehandTool) & KeyAction::StraightLineFromLastPoint) == KeyAction::StraightLineFromLastPoint &&
     document()->lastDrawingPoint() != Doc::NoLastDrawingPoint());
}

bool Editor::isSliceSelected(const doc::Slice* slice) const
{
  ASSERT(slice);
  return m_selectedSlices.contains(slice->id());
}

void Editor::clearSlicesSelection()
{
  if (!m_selectedSlices.empty()) {
    m_selectedSlices.clear();
    invalidate();

    if (isActive())
      UIContext::instance()->notifyActiveSiteChanged();
  }
}

void Editor::selectSlice(const doc::Slice* slice)
{
  ASSERT(slice);
  m_selectedSlices.insert(slice->id());
  invalidate();

  if (isActive())
    UIContext::instance()->notifyActiveSiteChanged();
}

bool Editor::selectSliceBox(const gfx::Rect& box)
{
  m_selectedSlices.clear();
  for (auto slice : m_sprite->slices()) {
    auto key = slice->getByFrame(m_frame);
    if (key && key->bounds().intersects(box))
      m_selectedSlices.insert(slice->id());
  }
  invalidate();

  if (isActive())
    UIContext::instance()->notifyActiveSiteChanged();

  return !m_selectedSlices.empty();
}

void Editor::selectAllSlices()
{
  for (auto slice : m_sprite->slices())
    m_selectedSlices.insert(slice->id());
  invalidate();

  if (isActive())
    UIContext::instance()->notifyActiveSiteChanged();
}

void Editor::cancelSelections()
{
  clearSlicesSelection();
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

    case kFocusEnterMessage: {
      ASSERT(m_state);
      if (m_state)
        m_state->onEditorGotFocus(this);
      break;
    }

    case kMouseEnterMessage:
      m_brushPreview.hide();
      updateToolLoopModifiersIndicators();
      updateQuicktool();
      break;

    case kMouseLeaveMessage:
      m_brushPreview.hide();
      StatusBar::instance()->showDefaultText();

      // Hide autoguides
      updateAutoCelGuides(nullptr);
      break;

    case kMouseDownMessage:
      if (m_sprite) {
        MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);

        // If we're going to start drawing, we cancel the flashing
        // layer.
        if (m_flashing != Flashing::None) {
          m_flashing = Flashing::None;
          invalidateCanvas();
        }

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
        bool state = m_state->onMouseDown(this, mouseMsg);

        // Re-update the tool modifiers if the state has changed
        // (e.g. we are on DrawingState now). This is required for the
        // Line tool to be able to Shift+press mouse buttons to start
        // drawing lines with the angle snapped.
        if (m_state != holdState)
          updateToolLoopModifiersIndicators(true);

        return state;
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
#if ENABLE_DEVMODE
      // Switch renderer
      if (msg->modifiers() == 0 &&
          static_cast<KeyMessage*>(msg)->scancode() == kKeyF1) {
        // TODO replace this experimental flag with a new enum (or
        //      maybe there is no need for user option now that the
        //      new engine allows to disable the bilinear mipmapping
        //      interpolation) as we still need the "old" engine to
        //      render reference layers
        auto& newRenderEngine = Preferences::instance().experimental.newRenderEngine;

#if SK_ENABLE_SKSL
        // Simple (new) -> Simple (old) -> Shader -> Simple (new) -> ...
        if (m_renderEngine->type() == EditorRender::Type::kShaderRenderer) {
          newRenderEngine(true);
          m_renderEngine->setType(EditorRender::Type::kSimpleRenderer);
        }
        else {
          if (newRenderEngine()) {
            newRenderEngine(false);
          }
          else {
            newRenderEngine(true);
            m_renderEngine->setType(EditorRender::Type::kShaderRenderer);
          }
        }
#else
        // Simple (new) <-> Simple (old)
        newRenderEngine(!newRenderEngine());
#endif

        switch (m_renderEngine->type()) {
          case EditorRender::Type::kSimpleRenderer:
            StatusBar::instance()->showTip(
              1000, fmt::format("Simple Renderer ({})", newRenderEngine() ? "new": "old"));
            break;
          case EditorRender::Type::kShaderRenderer:
            StatusBar::instance()->showTip(
              1000, fmt::format("Shader Renderer"));
            break;
        }

        app_refresh_screen();
        return true;
      }
#endif  // ENABLE_DEVMODE

      if (m_sprite && (isActive() || hasMouse())) {
        EditorStatePtr holdState(m_state);
        bool used = m_state->onKeyDown(this, static_cast<KeyMessage*>(msg));

        if (hasMouse()) {
          updateToolLoopModifiersIndicators();
          updateAutoCelGuides(msg);
          updateQuicktool();
          setCursor(mousePosInDisplay());
        }

        if (used)
          return true;
      }
      break;

    case kKeyUpMessage:
      if (m_sprite && (isActive() || hasMouse())) {
        EditorStatePtr holdState(m_state);
        bool used = m_state->onKeyUp(this, static_cast<KeyMessage*>(msg));

        if (hasMouse()) {
          updateToolLoopModifiersIndicators();
          updateAutoCelGuides(msg);
          updateQuicktool();
          setCursor(mousePosInDisplay());
        }

        if (used)
          return true;
      }
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

  bool result = Widget::onProcessMessage(msg);

  if (msg->type() == kPaintMessage &&
      m_flashing != Flashing::None) {
    const PaintMessage* ptmsg = static_cast<const PaintMessage*>(msg);
    if (ptmsg->count() == 0) {
      if (m_flashing == Flashing::WithFlashExtraCel) {
        m_flashing = Flashing::WaitingDeferedPaint;

        // We have to defer an invalidation so we can keep the
        // flashing layer in the extra cel some time.
        defer_invalid_rect(View::getView(this)->viewportBounds());
      }
      else if (m_flashing == Flashing::WaitingDeferedPaint) {
        m_flashing = Flashing::None;

        if (m_brushPreview.onScreen()) {
          m_brushPreview.hide();

          // Destroy the extra cel explicitly (it could happend
          // automatically by the m_brushPreview.show()) just in case
          // that the brush preview will not use the extra cel
          // (e.g. in the case of the Eraser tool).
          m_document->setExtraCel(ExtraCelRef(nullptr));

          showBrushPreview(mousePosInDisplay());
        }
        else {
          m_document->setExtraCel(ExtraCelRef(nullptr));
        }

        // Redraw all editors (without this the preview editor will
        // still show the flashing layer).
        for (auto editor : UIContext::instance()->getAllEditorsIncludingPreview(m_document)) {
          editor->invalidateCanvas();

          // Re-generate painting messages just right now (it looks
          // like the widget update region is lost after the last
          // kPaintMessage).
          editor->flushRedraw();
        }
      }
    }
  }

  return result;
}

void Editor::onSizeHint(SizeHintEvent& ev)
{
  gfx::Size sz(0, 0);
  if (m_sprite) {
    gfx::Point padding = calcExtraPadding(m_proj);
    gfx::Size canvas = canvasSize();
    sz.w = m_proj.applyX(canvas.w) + padding.x*2;
    sz.h = m_proj.applyY(canvas.h) + padding.y*2;
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
  std::unique_ptr<HideBrushPreview> hide;
  if (m_flashing == Flashing::None) {
    // If we are drawing the editor for a tooltip background or any
    // other semi-transparent widget (e.g. popups), we destroy the brush
    // preview/extra cel to avoid drawing a part of the brush in the
    // transparent widget background.
    if (ev.isTransparentBg()) {
      m_brushPreview.discardBrushPreview();
    }
    else {
      hide.reset(new HideBrushPreview(m_brushPreview));
    }
  }

  Graphics* g = ev.graphics();
  gfx::Rect rc = clientBounds();
  auto theme = SkinTheme::get(this);

  // Editor without sprite
  if (!m_sprite) {
    g->fillRect(theme->colors.editorFace(), rc);
  }
  // Editor with sprite
  else {
    try {
      // Lock the sprite to read/render it. Here we don't wait if the
      // document is locked (e.g. a filter is being applied to the
      // sprite) to avoid locking the UI.
      DocReader documentReader(m_document, 0);

      // Draw the sprite in the editor
      renderChrono.reset();
      drawBackground(g);
      drawSpriteUnclippedRect(g, gfx::Rect(0, 0, m_sprite->width(), m_sprite->height()));
      renderElapsed = renderChrono.elapsed();

#if ENABLE_DEVMODE
      // Show performance stats (TODO show performance stats in other widget)
      if (Preferences::instance().perf.showRenderTime()) {
        View* view = View::getView(this);
        gfx::Rect vp = view->viewportBounds();
        char buf[128];
        sprintf(buf, "%c %.4gs",
                Preferences::instance().experimental.newRenderEngine() ? 'N': 'O',
                renderElapsed);
        g->drawText(
          buf,
          gfx::rgba(255, 255, 255, 255),
          gfx::rgba(0, 0, 0, 255),
          vp.origin() - bounds().origin());

        m_perfInfoBounds.setOrigin(vp.origin());
        m_perfInfoBounds.setSize(g->measureUIText(buf));
      }
#endif // ENABLE_DEVMODE

      // Draw the mask boundaries
      if (m_document->hasMaskBoundaries()) {
        drawMask(g);
        m_antsTimer.start();
      }
      else {
        m_antsTimer.stop();
      }
    }
    catch (const LockedDocException&) {
      // The sprite is locked, so we cannot render it, we can draw an
      // opaque background now, and defer the rendering of the sprite
      // for later.
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
  if (hasMouse()) {
    updateStatusBar();
    setCursor(mousePosInDisplay());
  }
}

void Editor::onSamplingChange()
{
  if (m_proj.scaleX() < 1.0 &&
      m_proj.scaleY() < 1.0 &&
      isUsingNewRenderEngine()) {
    invalidate();
  }
}

void Editor::onFgColorChange()
{
  m_brushPreview.redraw();
}

void Editor::onContextBarBrushChange()
{
  m_brushPreview.redraw();
}

void Editor::onTiledModeBeforeChange()
{
  m_oldMainTilePos = mainTilePosition();
}

void Editor::onTiledModeChange()
{
  ASSERT(m_sprite);

  m_tiledModeHelper.mode(m_docPref.tiled.mode());

  // Get the sprite point in the middle of the editor, so we can
  // restore this with the new tiled mode in the main tile.
  View* view = View::getView(this);
  gfx::Rect vp = view->viewportBounds();
  gfx::Point screenPos(vp.x + vp.w/2,
                       vp.y + vp.h/2);
  gfx::Point spritePos(screenToEditor(screenPos));
  spritePos -= m_oldMainTilePos;

  // Update padding
  m_padding = calcExtraPadding(m_proj);

  spritePos += mainTilePosition();
  screenPos = editorToScreen(spritePos);

  centerInSpritePoint(spritePos);
}

void Editor::onShowExtrasChange()
{
  invalidate();
}

void Editor::onColorSpaceChanged(DocEvent& ev)
{
  // As the document has a new color space, we've to redraw the
  // complete canvas again with the new color profile.
  invalidate();
}

void Editor::onExposeSpritePixels(DocEvent& ev)
{
  if (m_state && ev.sprite() == m_sprite)
    m_state->onExposeSpritePixels(ev.region());
}

void Editor::onSpritePixelRatioChanged(DocEvent& ev)
{
  m_proj.setPixelRatio(ev.sprite()->pixelRatio());
  invalidate();
}

// TODO similar to ActiveSiteHandler::onBeforeRemoveLayer() and Timeline::onBeforeRemoveLayer()
void Editor::onBeforeRemoveLayer(DocEvent& ev)
{
  m_showGuidesThisCel = nullptr;

  // If the layer that was removed is the selected one in the editor,
  // or is an ancestor of the selected one.
  Layer* layerToSelect = candidate_if_layer_is_deleted(layer(), ev.layer());
  if (layer() != layerToSelect)
    setLayer(layerToSelect);
}

void Editor::onBeforeRemoveCel(DocEvent& ev)
{
  m_showGuidesThisCel = nullptr;
}

void Editor::onAddTag(DocEvent& ev)
{
  m_tagFocusBand = -1;
}

void Editor::onRemoveTag(DocEvent& ev)
{
  m_tagFocusBand = -1;
  if (m_state)
    m_state->onRemoveTag(this, ev.tag());
}

void Editor::onRemoveSlice(DocEvent& ev)
{
  ASSERT(ev.slice());
  if (ev.slice() &&
      m_selectedSlices.contains(ev.slice()->id())) {
    m_selectedSlices.erase(ev.slice()->id());
  }
}

void Editor::setCursor(const gfx::Point& mouseDisplayPos)
{
  Rect vp = View::getView(this)->viewportBounds();
  if (!vp.contains(mouseDisplayPos))
    return;

  bool used = false;
  if (m_sprite)
    used = m_state->onSetCursor(this, mouseDisplayPos);

  if (!used)
    showMouseCursor(kArrowCursor);
}

bool Editor::canDraw()
{
  return (m_layer != NULL &&
          m_layer->isImage() &&
          m_layer->isVisibleHierarchy() &&
          m_layer->isEditableHierarchy() &&
          !m_layer->isReference() &&
          !m_document->isReadOnly());
}

bool Editor::isInsideSelection()
{
  gfx::Point spritePos = screenToEditor(mousePosInDisplay());
  spritePos -= mainTilePosition();

  KeyAction action = m_customizationDelegate->getPressedKeyAction(KeyContext::SelectionTool);
  return
    (action == KeyAction::None) &&
    m_document &&
    m_document->isMaskVisible() &&
    m_document->mask()->containsPoint(spritePos.x, spritePos.y);
}

bool Editor::canStartMovingSelectionPixels()
{
  return
    isInsideSelection() &&
    // We cannot move the selection when add/subtract modes are
    // enabled (we prefer to modify the selection on those modes
    // instead of moving pixels).
    ((int(m_toolLoopModifiers) & int(tools::ToolLoopModifiers::kReplaceSelection)) ||
     // We can move the selection on add mode if the preferences says so.
     ((int(m_toolLoopModifiers) & int(tools::ToolLoopModifiers::kAddSelection)) &&
      Preferences::instance().selection.moveOnAddMode()) ||
     // We can move the selection when the Copy selection key (Ctrl) is pressed.
     (m_customizationDelegate &&
      int(m_customizationDelegate->getPressedKeyAction(KeyContext::TranslatingSelection) & KeyAction::CopySelection)));
}

bool Editor::keepTimelineRange()
{
  if (auto movingPixels = dynamic_cast<MovingPixelsState*>(m_state.get())) {
    if (movingPixels->canHandleFrameChange())
      return true;
  }
  return false;
}

EditorHit Editor::calcHit(const gfx::Point& mouseScreenPos)
{
  tools::Ink* ink = getCurrentEditorInk();

  if (ink) {
    // Check if we can transform slices
    if (ink->isSlice()) {
      if (m_docPref.show.slices()) {
        gfx::Point mainOffset(mainTilePosition());

        for (auto slice : m_sprite->slices()) {
          auto key = slice->getByFrame(m_frame);
          if (key) {
            gfx::Rect bounds = key->bounds();
            bounds.offset(mainOffset);
            bounds = editorToScreen(bounds);

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

  // Limit zooming screen position to the visible sprite bounds (we
  // use canvasSize() because if the tiled mode is enabled, we need
  // extra space for the zoom)
  gfx::Rect visibleBounds = editorToScreen(
    getViewportBounds().createIntersection(gfx::Rect(gfx::Point(0, 0), canvasSize())));
  screenPos.x = std::clamp(screenPos.x, visibleBounds.x, visibleBounds.x2()-1);
  screenPos.y = std::clamp(screenPos.y, visibleBounds.y, visibleBounds.y2()-1);

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
    updateEditor(false);
    setEditorScroll(scrollPos);
  }
}

void Editor::pasteImage(const Image* image, const Mask* mask)
{
  ASSERT(image);

  std::unique_ptr<Mask> temp_mask;
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
    const Rect visibleBounds = getViewportBounds();
    const Point maskCenter = mask->bounds().center();

    // If the pasted image original location center point isn't
    // visible, we center the image in the editor's visible bounds.
    if (maskCenter.x < visibleBounds.x ||
        maskCenter.x >= visibleBounds.x2()) {
      x = visibleBounds.x + visibleBounds.w/2 - image->width()/2;
    }
    // In other case, if the center is visible, we put the pasted
    // image in its original location.
    else {
      x = std::clamp(x, visibleBounds.x-image->width(), visibleBounds.x2()-1);
    }

    if (maskCenter.y < visibleBounds.y ||
        maskCenter.y >= visibleBounds.y2()) {
      y = visibleBounds.y + visibleBounds.h/2 - image->height()/2;
    }
    else {
      y = std::clamp(y, visibleBounds.y-image->height(), visibleBounds.y2()-1);
    }

    // Limit the image inside the sprite's bounds.
    if (sprite->width() <= image->width() ||
        sprite->height() <= image->height()) {
      x = std::clamp(x, 0, std::max(0, sprite->width() - image->width()));
      y = std::clamp(y, 0, std::max(0, sprite->height() - image->height()));
    }
    else {
      // Also we always limit the 1 image pixel inside the sprite's bounds.
      x = std::clamp(x, -image->width()+1, sprite->width()-1);
      y = std::clamp(y, -image->height()+1, sprite->height()-1);
    }
  }

  Site site = getSite();

  // Snap to grid a pasted tilemap
  // TODO should we move this to PixelsMovement or MovingPixelsState?
  if (site.tilemapMode() == TilemapMode::Tiles) {
    gfx::Rect gridBounds = site.gridBounds();
    gfx::Point pt = snap_to_grid(gridBounds,
                                 gfx::Point(x, y),
                                 PreferSnapTo::ClosestGridVertex);
    x = pt.x;
    y = pt.y;
  }

  // Clear brush preview, as the extra cel will be replaced with the
  // pasted image.
  m_brushPreview.hide();

  Mask mask2(*mask);
  mask2.setOrigin(x, y);

  PixelsMovementPtr pixelsMovement(
    new PixelsMovement(UIContext::instance(), site,
                       image, &mask2, "Paste"));

  setState(EditorStatePtr(new MovingPixelsState(this, NULL, pixelsMovement, NoHandle)));
}

void Editor::startSelectionTransformation(const gfx::Point& move, double angle)
{
  if (auto movingPixels = dynamic_cast<MovingPixelsState*>(m_state.get())) {
    movingPixels->translate(gfx::PointF(move));
    if (std::fabs(angle) > 1e-5)
      movingPixels->rotate(angle);
  }
  else if (auto standby = dynamic_cast<StandbyState*>(m_state.get())) {
    ASSERT(m_document->isMaskVisible());
    standby->startSelectionTransformation(this, move, angle);
  }
}

void Editor::startFlipTransformation(doc::algorithm::FlipType flipType)
{
  if (auto movingPixels = dynamic_cast<MovingPixelsState*>(m_state.get()))
    movingPixels->flip(flipType);
  else if (auto standby = dynamic_cast<StandbyState*>(m_state.get()))
    standby->startFlipTransformation(this, flipType);
}

void Editor::updateTransformation(const Transformation& transform)
{
  if (auto movingPixels = dynamic_cast<MovingPixelsState*>(m_state.get()))
    movingPixels->updateTransformation(transform);
}

void Editor::notifyScrollChanged()
{
  m_observers.notifyScrollChanged(this);

  ASSERT(m_state);
  if (m_state)
    m_state->onScrollChange(this);

  // Update status bar and mouse cursor
  if (hasMouse()) {
    updateStatusBar();
    setCursor(mousePosInDisplay());
  }
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
                  const bool playAll,
                  const bool playSubtags)
{
  ASSERT(m_state);
  if (!m_state)
    return;

  if (m_isPlaying)
    stop();

  m_isPlaying = true;
  setState(EditorStatePtr(new PlayState(playOnce,
                                        playAll,
                                        playSubtags)));
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

void Editor::showAnimationSpeedMultiplierPopup()
{
  const bool wasPlaying = isPlaying();

  if (auto menu = AppMenus::instance()->getAnimationMenu()) {
    UIContext::SetTargetView setView(m_docView);
    menu->showPopup(mousePosInDisplay(), display());
  }

  if (wasPlaying) {
    // Re-play
    stop();

    // TODO improve/generalize this in some way
    auto& pref = Preferences::instance();
    if (m_docView->isPreview()) {
      play(pref.preview.playOnce(),
           pref.preview.playAll(),
           pref.preview.playSubtags());
    }
    else {
      play(pref.editor.playOnce(),
           pref.editor.playAll(),
           pref.editor.playSubtags());
    }
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
  m_brushPreview.show(screenPos);
}

gfx::Point Editor::calcExtraPadding(const Projection& proj)
{
  View* view = View::getView(this);
  if (view) {
    Rect vp = view->viewportBounds();
    gfx::Size canvas = canvasSize();
    return gfx::Point(
      std::max<int>(vp.w/2, vp.w - proj.applyX(canvas.w)),
      std::max<int>(vp.h/2, vp.h - proj.applyY(canvas.h)));
  }
  else
    return gfx::Point(0, 0);
}

gfx::Size Editor::canvasSize() const
{
  return m_tiledModeHelper.canvasSize();
}

gfx::Point Editor::mainTilePosition() const
{
  return m_tiledModeHelper.mainTilePosition();
}

void Editor::expandRegionByTiledMode(gfx::Region& rgn,
                                     const bool withProj) const
{
  m_tiledModeHelper.expandRegionByTiledMode(rgn, withProj ? &m_proj : nullptr);
}

void Editor::collapseRegionByTiledMode(gfx::Region& rgn) const
{
  m_tiledModeHelper.collapseRegionByTiledMode(rgn);
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

void Editor::invalidateCanvas()
{
  if (!isVisible())
    return;

  if (m_sprite)
    invalidateRect(editorToScreen(getVisibleSpriteBounds()));
  else
    invalidate();
}

void Editor::invalidateIfActive()
{

  if (isActive())
    invalidate();
}

void Editor::updateAutoCelGuides(ui::Message* msg)
{
  Cel* oldShowGuidesThisCel = m_showGuidesThisCel;
  bool oldShowAutoCelGuides = m_showAutoCelGuides;

  m_showAutoCelGuides = (
    msg &&
    getCurrentEditorInk()->isCelMovement() &&
    m_docPref.show.autoGuides() &&
    m_customizationDelegate &&
    int(m_customizationDelegate->getPressedKeyAction(KeyContext::MoveTool) & KeyAction::AutoSelectLayer));

  // Check if the user is pressing the Ctrl or Cmd key on move
  // tool to show automatic guides.
  if (m_showAutoCelGuides &&
      m_state->allowLayerEdges()) {
    auto mouseMsg = dynamic_cast<ui::MouseMessage*>(msg);

    ColorPicker picker;
    picker.pickColor(getSite(),
                     screenToEditorF(mouseMsg ? mouseMsg->position():
                                                mousePosInDisplay()),
                     m_proj, ColorPicker::FromComposition);
    m_showGuidesThisCel = (picker.layer() ? picker.layer()->cel(m_frame):
                                            nullptr);
  }
  else {
    m_showGuidesThisCel = nullptr;
  }

  if (m_showGuidesThisCel != oldShowGuidesThisCel ||
      m_showAutoCelGuides != oldShowAutoCelGuides) {
    invalidate();
    updateStatusBar();
  }
}

// static
void Editor::registerCommands()
{
  Commands::instance()
    ->add(
      new QuickCommand(
        CommandId::SwitchNonactiveLayersOpacity(),
        []{
          static int oldValue = -1;
          auto& option = Preferences::instance().experimental.nonactiveLayersOpacity;
          if (oldValue == -1) {
            oldValue = option();
            if (option() == 255)
              option(128);
            else
              option(255);
          }
          else {
            const int newValue = oldValue;
            oldValue = option();
            option(newValue);
          }
          app_refresh_screen();
        }));
}

} // namespace app
