// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/tool_loop_impl.h"

#include "app/app.h"
#include "app/cmd/add_slice.h"
#include "app/cmd/set_last_point.h"
#include "app/cmd/set_mask.h"
#include "app/color.h"
#include "app/color_utils.h"
#include "app/console.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/doc_undo.h"
#include "app/i18n/strings.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/pref/preferences.h"
#include "app/tools/controller.h"
#include "app/tools/freehand_algorithm.h"
#include "app/tools/ink.h"
#include "app/tools/point_shape.h"
#include "app/tools/symmetries.h"
#include "app/tools/tool.h"
#include "app/tools/tool_box.h"
#include "app/tools/tool_loop.h"
#include "app/tx.h"
#include "app/ui/color_bar.h"
#include "app/ui/context_bar.h"
#include "app/ui/editor/editor.h"
#include "app/ui/main_window.h"
#include "app/ui/optional_alert.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "app/util/expand_cel_canvas.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/mask.h"
#include "doc/palette.h"
#include "doc/palette_picks.h"
#include "doc/remap.h"
#include "doc/slice.h"
#include "doc/sprite.h"
#include "fmt/format.h"
#include "render/dithering.h"
#include "render/render.h"
#include "ui/ui.h"

#include <algorithm>

namespace app {

using namespace ui;

#ifdef ENABLE_UI

static void fill_toolloop_params_from_tool_preferences(ToolLoopParams& params)
{
  ToolPreferences& toolPref =
    Preferences::instance().tool(params.tool);

  params.inkType = toolPref.ink();
  params.opacity = toolPref.opacity();
  params.tolerance = toolPref.tolerance();
  params.contiguous = toolPref.contiguous();
  params.freehandAlgorithm = toolPref.freehandAlgorithm();
}

#endif // ENABLE_UI

//////////////////////////////////////////////////////////////////////
// Common properties between drawing/preview ToolLoop impl

class ToolLoopBase : public tools::ToolLoop {

protected:
  Editor* m_editor;
  tools::Tool* m_tool;
  BrushRef m_brush;
  BrushRef m_origBrush;
  gfx::Point m_oldPatternOrigin;
  Doc* m_document;
  Sprite* m_sprite;
  Layer* m_layer;
  frame_t m_frame;
  RgbMap* m_rgbMap;
  DocumentPreferences& m_docPref;
  ToolPreferences& m_toolPref;
  int m_opacity;
  int m_tolerance;
  bool m_contiguous;
  bool m_snapToGrid;
  bool m_isSelectingTiles;
  gfx::Rect m_gridBounds;
  gfx::Point m_celOrigin;
  gfx::Point m_speed;
  tools::ToolLoop::Button m_button;
  std::unique_ptr<tools::Ink> m_ink;
  tools::Controller* m_controller;
  tools::PointShape* m_pointShape;
  tools::Intertwine* m_intertwine;
  tools::TracePolicy m_tracePolicy;
  std::unique_ptr<tools::Symmetry> m_symmetry;
  Shade m_shade;
  std::unique_ptr<doc::Remap> m_shadingRemap;
  app::ColorTarget m_colorTarget;
  doc::color_t m_fgColor;
  doc::color_t m_bgColor;
  doc::color_t m_primaryColor;
  doc::color_t m_secondaryColor;
  tools::DynamicsOptions m_dynamics;

public:
  ToolLoopBase(Editor* editor, Site site,
               ToolLoopParams& params)
    : m_editor(editor)
    , m_tool(params.tool)
    , m_brush(params.brush)
    , m_origBrush(params.brush)
    , m_oldPatternOrigin(m_brush->patternOrigin())
    , m_document(site.document())
    , m_sprite(site.sprite())
    , m_layer(site.layer())
    , m_frame(site.frame())
    , m_rgbMap(nullptr)
    , m_docPref(Preferences::instance().document(m_document))
    , m_toolPref(Preferences::instance().tool(m_tool))
    , m_opacity(params.opacity)
    , m_tolerance(params.tolerance)
    , m_contiguous(params.contiguous)
    , m_snapToGrid(m_docPref.grid.snap())
    , m_isSelectingTiles(false)
    , m_gridBounds(site.gridBounds())
    , m_button(params.button)
    , m_ink(params.ink->clone())
    , m_controller(params.controller)
    , m_pointShape(m_tool->getPointShape(m_button))
    , m_intertwine(m_tool->getIntertwine(m_button))
    , m_tracePolicy(m_tool->getTracePolicy(m_button))
    , m_symmetry(nullptr)
    , m_colorTarget(m_layer ? ColorTarget(m_layer):
                              ColorTarget(ColorTarget::BackgroundLayer,
                                          m_sprite->pixelFormat(),
                                          m_sprite->transparentColor()))
    , m_fgColor(color_utils::color_for_target_mask(params.fg, m_colorTarget))
    , m_bgColor(color_utils::color_for_target_mask(params.bg, m_colorTarget))
    , m_primaryColor(m_button == tools::ToolLoop::Left ? m_fgColor: m_bgColor)
    , m_secondaryColor(m_button == tools::ToolLoop::Left ? m_bgColor: m_fgColor)
  {
    ASSERT(m_tool);
    ASSERT(m_ink);
    ASSERT(m_controller);

#ifdef ENABLE_UI // TODO add dynamics support when UI is not enabled
    if (m_controller->isFreehand() &&
        !m_pointShape->isFloodFill() &&
        App::instance()->contextBar()) {
      m_dynamics = App::instance()->contextBar()->getDynamics();
    }
#endif

    if (m_tracePolicy == tools::TracePolicy::Accumulate ||
        m_tracePolicy == tools::TracePolicy::AccumulateUpdateLast) {
      tools::ToolBox* toolbox = App::instance()->toolBox();

      switch (params.freehandAlgorithm) {
        case tools::FreehandAlgorithm::DEFAULT:
          m_intertwine = toolbox->getIntertwinerById(tools::WellKnownIntertwiners::AsLines);
          m_tracePolicy = tools::TracePolicy::Accumulate;
          break;
        case tools::FreehandAlgorithm::PIXEL_PERFECT:
          m_intertwine = toolbox->getIntertwinerById(tools::WellKnownIntertwiners::AsPixelPerfect);
          m_tracePolicy = tools::TracePolicy::AccumulateUpdateLast;
          break;
        case tools::FreehandAlgorithm::DOTS:
          m_intertwine = toolbox->getIntertwinerById(tools::WellKnownIntertwiners::None);
          m_tracePolicy = tools::TracePolicy::Accumulate;
          break;
      }

      // Use overlap trace policy for dynamic gradient
      if (m_dynamics.isDynamic() &&
          m_dynamics.gradient != tools::DynamicSensor::Static &&
          m_controller->isFreehand() &&
          !m_ink->isEraser()) {
        // Use overlap trace policy to accumulate changes of colors
        // between stroke points.
        //
        // TODO this is connected with a condition in tools::PaintInk::prepareInk()
        m_tracePolicy = tools::TracePolicy::Overlap;
      }
    }

    // Symmetry mode
    if (Preferences::instance().symmetryMode.enabled()) {
      switch (m_docPref.symmetry.mode()) {

        case app::gen::SymmetryMode::NONE:
          ASSERT(m_symmetry == nullptr);
          break;

        case app::gen::SymmetryMode::HORIZONTAL:
          m_symmetry.reset(new app::tools::HorizontalSymmetry(m_docPref.symmetry.xAxis()));
          break;

        case app::gen::SymmetryMode::VERTICAL:
          m_symmetry.reset(new app::tools::VerticalSymmetry(m_docPref.symmetry.yAxis()));
          break;

        case app::gen::SymmetryMode::BOTH:
          m_symmetry.reset(
            new app::tools::SymmetryCombo(
              new app::tools::HorizontalSymmetry(m_docPref.symmetry.xAxis()),
              new app::tools::VerticalSymmetry(m_docPref.symmetry.yAxis())));
          break;
      }
    }

    // Ignore opacity for these inks
    if (!tools::inkHasOpacity(params.inkType) &&
        m_brush->type() != kImageBrushType &&
        !m_ink->isEffect()) {
      m_opacity = 255;
    }

#ifdef ENABLE_UI // TODO add support when UI is not enabled
    if (params.inkType == tools::InkType::SHADING) {
      m_shade = App::instance()->contextBar()->getShade();
      m_shadingRemap.reset(
        App::instance()->contextBar()->createShadeRemap(
          m_button == tools::ToolLoop::Left));
    }
#endif
  }

  ~ToolLoopBase() {
    m_origBrush->setPatternOrigin(m_oldPatternOrigin);
  }

  void forceSnapToTiles() {
    m_snapToGrid = true;
    m_isSelectingTiles = true;
  }

  // IToolLoop interface
  tools::Tool* getTool() override { return m_tool; }
  Brush* getBrush() override { return m_brush.get(); }
  void setBrush(const BrushRef& newBrush) override { m_brush = newBrush; }
  Doc* getDocument() override { return m_document; }
  Sprite* sprite() override { return m_sprite; }
  Layer* getLayer() override { return m_layer; }
  frame_t getFrame() override { return m_frame; }
  RgbMap* getRgbMap() override {
    if (!m_rgbMap) {
      Sprite::RgbMapFor forLayer =
        ((!m_layer ||
          m_layer->isBackground() ||
          m_sprite->pixelFormat() == IMAGE_RGB) ?
         Sprite::RgbMapFor::OpaqueLayer:
         Sprite::RgbMapFor::TransparentLayer);
      m_rgbMap = m_sprite->rgbMap(m_frame, forLayer);
    }
    return m_rgbMap;
  }
  ToolLoop::Button getMouseButton() override { return m_button; }
  doc::color_t getFgColor() override { return m_fgColor; }
  doc::color_t getBgColor() override { return m_bgColor; }
  doc::color_t getPrimaryColor() override { return m_primaryColor; }
  void setPrimaryColor(doc::color_t color) override { m_primaryColor = color; }
  doc::color_t getSecondaryColor() override { return m_secondaryColor; }
  void setSecondaryColor(doc::color_t color) override { m_secondaryColor = color; }
  int getOpacity() override { return m_opacity; }
  int getTolerance() override { return m_tolerance; }
  bool getContiguous() override { return m_contiguous; }
  tools::ToolLoopModifiers getModifiers() override {
    return m_editor ? m_editor->getToolLoopModifiers():
                      tools::ToolLoopModifiers::kNone;
  }
  filters::TiledMode getTiledMode() override { return m_docPref.tiled.mode(); }
  bool getGridVisible() override { return m_docPref.show.grid(); }
  bool getSnapToGrid() override { return m_snapToGrid; }
  bool isSelectingTiles() override { return m_isSelectingTiles; }
  bool getStopAtGrid() override {
    switch (m_toolPref.floodfill.stopAtGrid()) {
      case app::gen::StopAtGrid::NEVER:
        return false;
      case app::gen::StopAtGrid::IF_VISIBLE:
        return m_docPref.show.grid();
      case app::gen::StopAtGrid::ALWAYS:
        return true;
    }
    return false;
  }

  bool isPixelConnectivityEightConnected() override {
    return (m_toolPref.floodfill.pixelConnectivity()
            == app::gen::PixelConnectivity::EIGHT_CONNECTED);
  }

  gfx::Rect getGridBounds() override { return m_gridBounds; }
  gfx::Point getCelOrigin() override { return m_celOrigin; }
  void setSpeed(const gfx::Point& speed) override { m_speed = speed; }
  gfx::Point getSpeed() override { return m_speed; }
  tools::Ink* getInk() override { return m_ink.get(); }
  tools::Controller* getController() override { return m_controller; }
  tools::PointShape* getPointShape() override { return m_pointShape; }
  tools::Intertwine* getIntertwine() override { return m_intertwine; }
  tools::TracePolicy getTracePolicy() override {
    if (m_controller->handleTracePolicy())
      return m_controller->getTracePolicy();
    else
      return m_tracePolicy;
  }
  tools::Symmetry* getSymmetry() override { return m_symmetry.get(); }
  const Shade& getShade() override { return m_shade; }
  doc::Remap* getShadingRemap() override { return m_shadingRemap.get(); }

  void limitDirtyAreaToViewport(gfx::Region& rgn) override {
#ifdef ENABLE_UI
    // Visible region (on the screen) of the all editors showing the
    // given document.
    gfx::Region allVisibleRgn;

    // TODO use the context given to the ToolLoopImpl ctor
    for (auto e : UIContext::instance()->getAllEditorsIncludingPreview(m_document)) {
      gfx::Region viewportRegion;
      e->getDrawableRegion(viewportRegion, Widget::kCutTopWindows);
      for (auto rc : viewportRegion) {
        gfx::Region subrgn(e->screenToEditor(rc).inflate(1, 1));
        e->collapseRegionByTiledMode(subrgn);
        allVisibleRgn |= subrgn;
      }
    }

    rgn &= allVisibleRgn;
#endif // ENABLE_UI
  }

  void updateDirtyArea(const gfx::Region& dirtyArea) override {
    if (!m_editor)
      return;

#ifdef ENABLE_UI
    // This is necessary here so the "on sprite crosshair" is hidden,
    // we update screen pixels with the new sprite, and then we show
    // the crosshair saving the updated pixels. It fixes problems with
    // filled shape tools when we release the button, or paint-bucket
    // when we press the button.
    HideBrushPreview hide(m_editor->brushPreview());
#endif

    m_document->notifySpritePixelsModified(
      m_sprite, dirtyArea, m_frame);
  }

  void updateStatusBar(const char* text) override {
#ifdef ENABLE_UI
    if (auto statusBar = StatusBar::instance())
      statusBar->setStatusText(0, text);
#endif
  }

  gfx::Point statusBarPositionOffset() override {
#ifdef ENABLE_UI
    return (m_editor ? -m_editor->mainTilePosition(): gfx::Point(0, 0));
#else
    return gfx::Point(0, 0);
#endif
  }

  render::DitheringMatrix getDitheringMatrix() override {
#ifdef ENABLE_UI // TODO add support when UI is not enabled
    return App::instance()->contextBar()->ditheringMatrix();
#else
    return render::DitheringMatrix();
#endif
  }

  render::DitheringAlgorithmBase* getDitheringAlgorithm() override {
#ifdef ENABLE_UI // TODO add support when UI is not enabled
    return App::instance()->contextBar()->ditheringAlgorithm();
#else
    return nullptr;
#endif
  }

  render::GradientType getGradientType() override {
#ifdef ENABLE_UI // TODO add support when UI is not enabled
    return App::instance()->contextBar()->gradientType();
#else
    return render::GradientType::Linear;
#endif
  }

  tools::DynamicsOptions getDynamics() override {
    return m_dynamics;
  }

  void onSliceRect(const gfx::Rect& bounds) override { }

};

//////////////////////////////////////////////////////////////////////
// For drawing

class ToolLoopImpl : public ToolLoopBase {
  Context* m_context;
  bool m_filled;
  bool m_previewFilled;
  int m_sprayWidth;
  int m_spraySpeed;
  bool m_useMask;
  Mask* m_mask;
  gfx::Point m_maskOrigin;
  bool m_canceled;
  Tx m_tx;
  ExpandCelCanvas* m_expandCelCanvas;
  Image* m_floodfillSrcImage;
  bool m_saveLastPoint;

public:
  ToolLoopImpl(Editor* editor,
               Site site,
               Context* context,
               ToolLoopParams& params,
               const bool saveLastPoint)
    : ToolLoopBase(editor, site, params)
    , m_context(context)
    , m_canceled(false)
    , m_tx(m_context,
           m_tool->getText().c_str(),
           ((getInk()->isSelection() ||
             getInk()->isEyedropper() ||
             getInk()->isScrollMovement() ||
             getInk()->isSlice() ||
             getInk()->isZoom()) ? DoesntModifyDocument:
                                   ModifyDocument))
    , m_expandCelCanvas(nullptr)
    , m_floodfillSrcImage(nullptr)
    , m_saveLastPoint(saveLastPoint)
  {
    if (m_pointShape->isFloodFill()) {
      // Prepare a special image for floodfill when it's configured to
      // stop using all visible layers.
      if (m_toolPref.floodfill.referTo() == gen::FillReferTo::ALL_LAYERS) {
        m_floodfillSrcImage = Image::create(m_sprite->pixelFormat(),
                                            m_sprite->width(),
                                            m_sprite->height());

        m_floodfillSrcImage->clear(m_sprite->transparentColor());

        render::Render render;
        render.setNewBlend(Preferences::instance().experimental.newBlend());
        render.renderSprite(
          m_floodfillSrcImage,
          m_sprite,
          m_frame,
          gfx::Clip(m_sprite->bounds()));
      }
      else {
        Cel* cel = m_layer->cel(m_frame);
        if (cel && (cel->x() != 0 || cel->y() != 0)) {
          m_floodfillSrcImage = Image::create(m_sprite->pixelFormat(),
                                              m_sprite->width(),
                                              m_sprite->height());
          m_floodfillSrcImage->clear(m_sprite->transparentColor());
          copy_image(m_floodfillSrcImage, cel->image(), cel->x(), cel->y());
        }
      }
    }

    m_expandCelCanvas = new ExpandCelCanvas(
      site, site.layer(),
      m_docPref.tiled.mode(),
      m_tx,
      ExpandCelCanvas::Flags(
        ExpandCelCanvas::NeedsSource |
        // If the tool is freehand-like, we can use the modified
        // region directly as undo information to save the modified
        // pixels. See ExpandCelCanvas::commit() for details about this flag.
        (getController()->isFreehand() ?
         ExpandCelCanvas::UseModifiedRegionAsUndoInfo:
         ExpandCelCanvas::None)));

    if (!m_floodfillSrcImage)
      m_floodfillSrcImage = const_cast<Image*>(getSrcImage());

    // Settings
    switch (m_tool->getFill(m_button)) {
      case tools::FillNone:
        m_filled = false;
        break;
      case tools::FillAlways:
        m_filled = true;
        break;
      case tools::FillOptional:
        m_filled = m_toolPref.filled();
        break;
    }

    m_previewFilled = m_toolPref.filledPreview();
    m_sprayWidth = m_toolPref.spray.width();
    m_spraySpeed = m_toolPref.spray.speed();

    if (m_ink->isSelection())
      m_useMask = false;
    else
      m_useMask = m_document->isMaskVisible();

    // Start with an empty mask if the user is selecting with "default selection mode"
    if (getInk()->isSelection() &&
        (!m_document->isMaskVisible() ||
         (int(getModifiers()) & int(tools::ToolLoopModifiers::kReplaceSelection)))) {
      Mask emptyMask;
      m_tx(new cmd::SetMask(m_document, &emptyMask));
    }

    m_celOrigin = m_expandCelCanvas->getCel()->position();
    m_mask = m_document->mask();
    m_maskOrigin = (!m_mask->isEmpty() ? gfx::Point(m_mask->bounds().x-m_celOrigin.x,
                                                    m_mask->bounds().y-m_celOrigin.y):
                                         gfx::Point(0, 0));
  }

  ~ToolLoopImpl() {
    if (m_floodfillSrcImage != getSrcImage())
      delete m_floodfillSrcImage;
    delete m_expandCelCanvas;
  }

  // IToolLoop interface
  void commitOrRollback() override {
    bool redraw = false;

    if (!m_canceled) {
      // Freehand changes the last point
      if (m_saveLastPoint) {
        m_tx(new cmd::SetLastPoint(
               m_document,
               getController()->getLastPoint().toPoint()));
      }

      // Paint ink
      if (getInk()->isPaint()) {
        try {
          ContextReader reader(m_context, 500);
          ContextWriter writer(reader);
          m_expandCelCanvas->commit();
        }
        catch (const LockedDocException& ex) {
          Console::showException(ex);
        }
      }
      // Selection ink
      else if (getInk()->isSelection()) {
        redraw = true;

        // Show selection edges
        if (Preferences::instance().selection.autoShowSelectionEdges())
          m_docPref.show.selectionEdges(true);
      }
      // Slice ink
      else if (getInk()->isSlice()) {
        redraw = true;
      }

      m_tx.commit();
    }
    else {
      redraw = true;
    }

    // If the trace was canceled or it is not a 'paint' ink...
    if (m_canceled || !getInk()->isPaint()) {
      try {
        ContextReader reader(m_context, 500);
        ContextWriter writer(reader);
        m_expandCelCanvas->rollback();
      }
      catch (const LockedDocException& ex) {
        Console::showException(ex);
      }
    }

#ifdef ENABLE_UI
    if (redraw)
      update_screen_for_document(m_document);
#else
    (void)redraw;               // To avoid warning about unused variable
#endif
  }

  const Image* getSrcImage() override { return m_expandCelCanvas->getSourceCanvas(); }
  const Image* getFloodFillSrcImage() override { return m_floodfillSrcImage; }
  Image* getDstImage() override { return m_expandCelCanvas->getDestCanvas(); }
  void validateSrcImage(const gfx::Region& rgn) override {
    m_expandCelCanvas->validateSourceCanvas(rgn);
  }
  void validateDstImage(const gfx::Region& rgn) override {
    m_expandCelCanvas->validateDestCanvas(rgn);
  }
  void invalidateDstImage() override {
    m_expandCelCanvas->invalidateDestCanvas();
  }
  void invalidateDstImage(const gfx::Region& rgn) override {
    m_expandCelCanvas->invalidateDestCanvas(rgn);
  }
  void copyValidDstToSrcImage(const gfx::Region& rgn) override {
    m_expandCelCanvas->copyValidDestToSourceCanvas(rgn);
  }

  bool useMask() override { return m_useMask; }
  Mask* getMask() override { return m_mask; }
  void setMask(Mask* newMask) override {
    m_tx(new cmd::SetMask(m_document, newMask));
  }
  gfx::Point getMaskOrigin() override { return m_maskOrigin; }
  bool getFilled() override { return m_filled; }
  bool getPreviewFilled() override { return m_previewFilled; }
  int getSprayWidth() override { return m_sprayWidth; }
  int getSpraySpeed() override { return m_spraySpeed; }

  void cancel() override { m_canceled = true; }
  bool isCanceled() override { return m_canceled; }

  void onSliceRect(const gfx::Rect& bounds) override {
#ifdef ENABLE_UI // TODO add support for slice tool from batch scripts without UI?
    if (m_editor && getMouseButton() == ToolLoop::Left) {
      // Try to select slices, but if it returns false, it means that
      // there are no slices in the box to be selected, so we show a
      // popup menu to create a new one.
      if (!m_editor->selectSliceBox(bounds) &&
          (bounds.w > 1 || bounds.h > 1)) {
        Slice* slice = new Slice;
        slice->setName(getUniqueSliceName());

        SliceKey key(bounds);
        slice->insert(getFrame(), key);

        auto color = Preferences::instance().slices.defaultColor();
        slice->userData().setColor(
          doc::rgba(color.getRed(),
                    color.getGreen(),
                    color.getBlue(),
                    color.getAlpha()));

        m_tx(new cmd::AddSlice(m_sprite, slice));
        return;
      }
    }
#endif

    // Cancel the operation (do not create a new transaction for this
    // no-op, e.g. just change the set of selected slices).
    m_canceled = true;
  }

private:

#ifdef ENABLE_UI
  std::string getUniqueSliceName() const {
    std::string prefix = "Slice";
    int max = 0;

    for (Slice* slice : m_sprite->slices())
      if (std::strncmp(slice->name().c_str(), prefix.c_str(), prefix.size()) == 0)
        max = std::max(max, (int)std::strtol(slice->name().c_str()+prefix.size(), nullptr, 10));

    return fmt::format("{} {}", prefix, max+1);
  }
#endif

};

//////////////////////////////////////////////////////////////////////
// For user UI painting

#ifdef ENABLE_UI

tools::ToolLoop* create_tool_loop(
  Editor* editor,
  Context* context,
  const tools::Pointer::Button button,
  const bool convertLineToFreehand,
  const bool selectTiles)
{
  ToolLoopParams params;
  params.tool = editor->getCurrentEditorTool();
  params.ink = editor->getCurrentEditorInk();
  if (!params.tool || !params.ink)
    return nullptr;

  if (selectTiles) {
    params.tool = App::instance()->toolBox()->getToolById(tools::WellKnownTools::RectangularMarquee);
    params.ink = params.tool->getInk(button == tools::Pointer::Left ? 0: 1);
  }

  Site site = editor->getSite();

  // For selection tools, we can use any layer (even without layers at
  // all), so we specify a nullptr here as the active layer. This is
  // used as a special case by the render::Render class to show the
  // preview image/selection stroke as a xor'd overlay in the render
  // result.
  //
  // Anyway this cannot be used in 'magic wand' tool (isSelection +
  // isFloodFill) because we need the original layer source
  // image/pixels to stop the flood-fill algorithm.
  if (params.ink->isSelection() &&
      !params.tool->getPointShape(
        button != tools::Pointer::Left ? 1: 0)->isFloodFill()) {
    site.layer(nullptr);
  }
  else {
    Layer* layer = site.layer();
    if (!layer) {
      StatusBar::instance()->showTip(
        1000, "There is no active layer");
      return nullptr;
    }
    else if (!layer->isVisibleHierarchy()) {
      StatusBar::instance()->showTip(
        1000, fmt::format("Layer '{}' is hidden", layer->name()));
      return nullptr;
    }
    // If the active layer is read-only.
    else if (!layer->isEditableHierarchy()) {
      StatusBar::instance()->showTip(
        1000, fmt::format("Layer '{}' is locked", layer->name()));
      return nullptr;
    }
    // If the active layer is reference.
    else if (layer->isReference()) {
      StatusBar::instance()->showTip(
        1000, fmt::format("Layer '{}' is reference, cannot be modified", layer->name()));
      return nullptr;
    }
  }

  // Get fg/bg colors
  ColorBar* colorbar = ColorBar::instance();
  params.fg = colorbar->getFgColor();
  params.bg = colorbar->getBgColor();

  if (!params.fg.isValid() ||
      !params.bg.isValid()) {
    if (Preferences::instance().colorBar.showInvalidFgBgColorAlert()) {
      OptionalAlert::show(
        Preferences::instance().colorBar.showInvalidFgBgColorAlert,
        1, Strings::alerts_invalid_fg_or_bg_colors());
      return nullptr;
    }
  }

  // Create the new tool loop
  try {
    params.button =
      (button == tools::Pointer::Left ? tools::ToolLoop::Left:
                                        tools::ToolLoop::Right);

    params.controller =
      (convertLineToFreehand ?
       App::instance()->toolBox()->getControllerById(
         tools::WellKnownControllers::LineFreehand):
       params.tool->getController(params.button));

    const bool saveLastPoint =
      (params.ink->isPaint() &&
       (params.controller->isFreehand() ||
        convertLineToFreehand));

    params.brush = App::instance()->contextBar()
      ->activeBrush(params.tool, params.ink);

    fill_toolloop_params_from_tool_preferences(params);

    ASSERT(context->activeDocument() == editor->document());
    auto toolLoop = new ToolLoopImpl(
      editor, site, context, params, saveLastPoint);

    if (selectTiles)
      toolLoop->forceSnapToTiles();

    return toolLoop;
  }
  catch (const std::exception& ex) {
    Console::showException(ex);
    return NULL;
  }
}

#endif // ENABLE_UI

//////////////////////////////////////////////////////////////////////
// For scripting

#ifdef ENABLE_SCRIPTING

tools::ToolLoop* create_tool_loop_for_script(
  Context* context,
  const Site& site,
  ToolLoopParams& params)
{
  ASSERT(params.tool);
  ASSERT(params.ink);
  if (!site.layer())
    return nullptr;

  try {
    // If we don't have the UI available, we reset the tools
    // preferences, so scripts that are executed in batch mode have a
    // reproducible behavior.
    if (!context->isUIAvailable())
      Preferences::instance().resetToolPreferences(params.tool);

    return new ToolLoopImpl(
      nullptr, site, context, params, false);
  }
  catch (const std::exception& ex) {
    Console::showException(ex);
    return nullptr;
  }
}

#endif // ENABLE_SCRIPTING

//////////////////////////////////////////////////////////////////////
// For UI preview

#ifdef ENABLE_UI

class PreviewToolLoopImpl : public ToolLoopBase {
  Image* m_image;

public:
  PreviewToolLoopImpl(
    Editor* editor,
    ToolLoopParams& params,
    Image* image,
    const gfx::Point& celOrigin)
    : ToolLoopBase(editor, editor->getSite(), params)
    , m_image(image)
  {
    m_celOrigin = celOrigin;

    // Avoid preview for spray and flood fill like tools
    if (m_pointShape->isSpray()) {
      m_pointShape = App::instance()->toolBox()->getPointShapeById(
        tools::WellKnownPointShapes::Brush);
    }
    else if (m_pointShape->isFloodFill()) {
      m_pointShape = App::instance()->toolBox()->getPointShapeById(
        tools::WellKnownPointShapes::Pixel);
    }
  }

  // IToolLoop interface
  void commitOrRollback() override {
    // Do nothing
  }
  const Image* getSrcImage() override { return m_image; }
  const Image* getFloodFillSrcImage() override { return m_image; }
  Image* getDstImage() override { return m_image; }
  void validateSrcImage(const gfx::Region& rgn) override { }
  void validateDstImage(const gfx::Region& rgn) override { }
  void invalidateDstImage() override { }
  void invalidateDstImage(const gfx::Region& rgn) override { }
  void copyValidDstToSrcImage(const gfx::Region& rgn) override { }

  bool useMask() override { return false; }
  Mask* getMask() override { return nullptr; }
  void setMask(Mask* newMask) override { }
  gfx::Point getMaskOrigin() override { return gfx::Point(0, 0); }
  bool getFilled() override { return false; }
  bool getPreviewFilled() override { return false; }
  int getSprayWidth() override { return 0; }
  int getSpraySpeed() override { return 0; }

  void cancel() override { }
  bool isCanceled() override { return true; }

  tools::DynamicsOptions getDynamics() override {
    // Preview without dynamics
    return tools::DynamicsOptions();
  }

};

tools::ToolLoop* create_tool_loop_preview(
  Editor* editor,
  const doc::BrushRef& brush,
  Image* image,
  const gfx::Point& celOrigin)
{
  ToolLoopParams params;
  params.tool = editor->getCurrentEditorTool();
  params.ink = editor->getCurrentEditorInk();
  if (!params.tool || !params.ink)
    return nullptr;

  Layer* layer = editor->layer();
  if (!layer ||
      !layer->isVisibleHierarchy() ||
      !layer->isEditableHierarchy() ||
      layer->isReference()) {
    return nullptr;
  }

  // Get fg/bg colors
  ColorBar* colorbar = ColorBar::instance();
  params.fg = colorbar->getFgColor();
  params.bg = colorbar->getBgColor();
  if (!params.fg.isValid() ||
      !params.bg.isValid())
    return nullptr;

  params.brush = brush;
  params.button = tools::ToolLoop::Left;
  params.controller = params.tool->getController(params.button);

  // Create the new tool loop
  try {
    fill_toolloop_params_from_tool_preferences(params);

    return new PreviewToolLoopImpl(
      editor, params, image, celOrigin);
  }
  catch (const std::exception& e) {
    LOG(ERROR, e.what());
    return nullptr;
  }
}

#endif // ENABLE_UI

//////////////////////////////////////////////////////////////////////

} // namespace app
