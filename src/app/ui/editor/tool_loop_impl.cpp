// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/tool_loop_impl.h"

#include "app/app.h"
#include "app/cmd/set_mask.h"
#include "app/color.h"
#include "app/color_utils.h"
#include "app/console.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/document_undo.h"
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
#include "app/transaction.h"
#include "app/ui/color_bar.h"
#include "app/ui/context_bar.h"
#include "app/ui/editor/editor.h"
#include "app/ui/main_window.h"
#include "app/ui/status_bar.h"
#include "app/util/expand_cel_canvas.h"
#include "doc/brush.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/mask.h"
#include "doc/palette.h"
#include "doc/palette_picks.h"
#include "doc/remap.h"
#include "doc/sprite.h"
#include "ui/ui.h"

namespace app {

using namespace ui;

//////////////////////////////////////////////////////////////////////
// Common properties between drawing/preview ToolLoop impl

class ToolLoopBase : public tools::ToolLoop {

protected:
  Editor* m_editor;
  tools::Tool* m_tool;
  BrushRef m_brush;
  Document* m_document;
  Sprite* m_sprite;
  Layer* m_layer;
  frame_t m_frame;
  DocumentPreferences& m_docPref;
  ToolPreferences& m_toolPref;
  int m_opacity;
  int m_tolerance;
  bool m_contiguous;
  gfx::Point m_offset;
  gfx::Point m_speed;
  tools::ToolLoop::Button m_button;
  base::UniquePtr<tools::Ink> m_ink;
  tools::Controller* m_controller;
  tools::PointShape* m_pointShape;
  tools::Intertwine* m_intertwine;
  tools::TracePolicy m_tracePolicy;
  base::UniquePtr<tools::Symmetry> m_symmetry;
  base::UniquePtr<doc::Remap> m_shadingRemap;
  doc::color_t m_fgColor;
  doc::color_t m_bgColor;
  doc::color_t m_primaryColor;
  doc::color_t m_secondaryColor;
  gfx::Region m_dirtyArea;

public:
  ToolLoopBase(Editor* editor,
               tools::Tool* tool,
               tools::Ink* ink,
               Document* document,
               tools::ToolLoop::Button button,
               const app::Color& fgColor,
               const app::Color& bgColor)
    : m_editor(editor)
    , m_tool(tool)
    , m_brush(App::instance()->getMainWindow()->getContextBar()->activeBrush())
    , m_document(document)
    , m_sprite(editor->sprite())
    , m_layer(editor->layer())
    , m_frame(editor->frame())
    , m_docPref(Preferences::instance().document(m_document))
    , m_toolPref(Preferences::instance().tool(m_tool))
    , m_opacity(m_toolPref.opacity())
    , m_tolerance(m_toolPref.tolerance())
    , m_contiguous(m_toolPref.contiguous())
    , m_button(button)
    , m_ink(ink->clone())
    , m_controller(m_tool->getController(m_button))
    , m_pointShape(m_tool->getPointShape(m_button))
    , m_intertwine(m_tool->getIntertwine(m_button))
    , m_tracePolicy(m_tool->getTracePolicy(m_button))
    , m_symmetry(nullptr)
    , m_fgColor(color_utils::color_for_target_mask(fgColor, ColorTarget(m_layer)))
    , m_bgColor(color_utils::color_for_target_mask(bgColor, ColorTarget(m_layer)))
    , m_primaryColor(button == tools::ToolLoop::Left ? m_fgColor: m_bgColor)
    , m_secondaryColor(button == tools::ToolLoop::Left ? m_bgColor: m_fgColor)
  {
    tools::FreehandAlgorithm algorithm = m_toolPref.freehandAlgorithm();

    if (m_tracePolicy == tools::TracePolicy::Accumulate ||
        m_tracePolicy == tools::TracePolicy::AccumulateUpdateLast) {
      tools::ToolBox* toolbox = App::instance()->getToolBox();

      switch (algorithm) {
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
    }

    // Symmetry mode
    switch (m_docPref.symmetry.mode()) {

      case app::gen::SymmetryMode::NONE:
        ASSERT(m_symmetry == nullptr);
        break;

      case app::gen::SymmetryMode::HORIZONTAL:
        if (m_docPref.symmetry.xAxis() == 0)
          m_docPref.symmetry.xAxis(m_sprite->width()/2);

        m_symmetry.reset(new app::tools::HorizontalSymmetry(m_docPref.symmetry.xAxis()));
        break;

      case app::gen::SymmetryMode::VERTICAL:
        if (m_docPref.symmetry.yAxis() == 0)
          m_docPref.symmetry.yAxis(m_sprite->height()/2);

        m_symmetry.reset(new app::tools::VerticalSymmetry(m_docPref.symmetry.yAxis()));
        break;
    }

    // Ignore opacity for these inks
    if (!tools::inkHasOpacity(m_toolPref.ink()) &&
        m_brush->type() != kImageBrushType &&
        !m_ink->isEffect()) {
      m_opacity = 255;
    }

    if (m_toolPref.ink() == tools::InkType::SHADING) {
      m_shadingRemap.reset(
        App::instance()->getMainWindow()->getContextBar()->createShadesRemap(
          button == tools::ToolLoop::Left));
    }
  }

  // IToolLoop interface
  tools::Tool* getTool() override { return m_tool; }
  Brush* getBrush() override { return m_brush.get(); }
  Document* getDocument() override { return m_document; }
  Sprite* sprite() override { return m_sprite; }
  Layer* getLayer() override { return m_layer; }
  frame_t getFrame() override { return m_frame; }
  RgbMap* getRgbMap() override { return m_sprite->rgbMap(m_frame); }
  const render::Zoom& zoom() override { return m_editor->zoom(); }
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
  tools::SelectionMode getSelectionMode() override { return m_editor->getSelectionMode(); }
  filters::TiledMode getTiledMode() override { return m_docPref.tiled.mode(); }
  bool getGridVisible() override { return m_docPref.grid.visible(); }
  bool getSnapToGrid() override { return m_docPref.grid.snap(); }
  bool getStopAtGrid() override {
    switch (m_toolPref.floodfill.stopAtGrid()) {
      case app::gen::StopAtGrid::NEVER:
        return false;
      case app::gen::StopAtGrid::IF_VISIBLE:
        return m_docPref.grid.visible();
      case app::gen::StopAtGrid::ALWAYS:
        return true;
    }
    return false;
  }
  gfx::Rect getGridBounds() override { return m_docPref.grid.bounds(); }
  gfx::Point getOffset() override { return m_offset; }
  void setSpeed(const gfx::Point& speed) override { m_speed = speed; }
  gfx::Point getSpeed() override { return m_speed; }
  tools::Ink* getInk() override { return m_ink; }
  tools::Controller* getController() override { return m_controller; }
  tools::PointShape* getPointShape() override { return m_pointShape; }
  tools::Intertwine* getIntertwine() override { return m_intertwine; }
  tools::TracePolicy getTracePolicy() override { return m_tracePolicy; }
  tools::Symmetry* getSymmetry() override { return m_symmetry.get(); }
  doc::Remap* getShadingRemap() override { return m_shadingRemap; }

  gfx::Region& getDirtyArea() override {
    return m_dirtyArea;
  }

  void updateDirtyArea() override {
    // TODO find a way to avoid calling hide/show brush preview here
    HideBrushPreview hide(m_editor->brushPreview());
    m_document->notifySpritePixelsModified(m_sprite, m_dirtyArea,
                                           m_frame);
  }

  void updateStatusBar(const char* text) override {
    StatusBar::instance()->setStatusText(0, text);
  }

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
  Transaction m_transaction;
  ExpandCelCanvas m_expandCelCanvas;

public:
  ToolLoopImpl(Editor* editor,
               Context* context,
               tools::Tool* tool,
               tools::Ink* ink,
               Document* document,
               tools::ToolLoop::Button button,
               const app::Color& fgColor,
               const app::Color& bgColor)
    : ToolLoopBase(editor, tool, ink, document,
                   button, fgColor, bgColor)
    , m_context(context)
    , m_canceled(false)
    , m_transaction(m_context,
                    m_tool->getText().c_str(),
                    ((getInk()->isSelection() ||
                      getInk()->isEyedropper() ||
                      getInk()->isScrollMovement() ||
                      getInk()->isSlice() ||
                      getInk()->isZoom()) ? DoesntModifyDocument:
                                            ModifyDocument))
    , m_expandCelCanvas(editor->getSite(),
        m_docPref.tiled.mode(),
        m_transaction,
        ExpandCelCanvas::Flags(
          ExpandCelCanvas::NeedsSource |
          // If the tool is freehand-like, we can use the modified
          // region directly as undo information to save the modified
          // pixels (it's faster than creating a Dirty object).
          // See ExpandCelCanvas::commit() for details about this flag.
          (getController()->isFreehand() ?
            ExpandCelCanvas::UseModifiedRegionAsUndoInfo:
            ExpandCelCanvas::None)))
  {
    ASSERT(m_context->activeDocument() == m_editor->document());

    // Settings
    switch (tool->getFill(m_button)) {
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
         getSelectionMode() == tools::SelectionMode::DEFAULT)) {
      Mask emptyMask;
      m_transaction.execute(new cmd::SetMask(m_document, &emptyMask));
    }

    int x1 = m_expandCelCanvas.getCel()->x();
    int y1 = m_expandCelCanvas.getCel()->y();

    m_mask = m_document->mask();
    m_maskOrigin = (!m_mask->isEmpty() ? gfx::Point(m_mask->bounds().x-x1,
                                                    m_mask->bounds().y-y1):
                                         gfx::Point(0, 0));

    m_offset.x = -x1;
    m_offset.y = -y1;
  }

  // IToolLoop interface
  void dispose() override
  {
    bool redraw = false;

    if (!m_canceled) {
      // Paint ink
      if (getInk()->isPaint()) {
        try {
          ContextReader reader(m_context, 500);
          ContextWriter writer(reader, 500);
          m_expandCelCanvas.commit();
        }
        catch (const LockedDocumentException& ex) {
          Console::showException(ex);
        }
      }
      // Selection ink
      else if (getInk()->isSelection()) {
        m_document->generateMaskBoundaries();
        redraw = true;
      }

      m_transaction.commit();
    }
    else
      redraw = true;

    // If the trace was canceled or it is not a 'paint' ink...
    if (m_canceled || !getInk()->isPaint()) {
      try {
        ContextReader reader(m_context, 500);
        ContextWriter writer(reader, 500);
        m_expandCelCanvas.rollback();
      }
      catch (const LockedDocumentException& ex) {
        Console::showException(ex);
      }
    }

    if (redraw)
      update_screen_for_document(m_document);
  }

  const Image* getSrcImage() override { return m_expandCelCanvas.getSourceCanvas(); }
  Image* getDstImage() override { return m_expandCelCanvas.getDestCanvas(); }
  void validateSrcImage(const gfx::Region& rgn) override {
    m_expandCelCanvas.validateSourceCanvas(rgn);
  }
  void validateDstImage(const gfx::Region& rgn) override {
    m_expandCelCanvas.validateDestCanvas(rgn);
  }
  void invalidateDstImage() override {
    m_expandCelCanvas.invalidateDestCanvas();
  }
  void invalidateDstImage(const gfx::Region& rgn) override {
    m_expandCelCanvas.invalidateDestCanvas(rgn);
  }
  void copyValidDstToSrcImage(const gfx::Region& rgn) override {
    m_expandCelCanvas.copyValidDestToSourceCanvas(rgn);
  }

  bool useMask() override { return m_useMask; }
  Mask* getMask() override { return m_mask; }
  void setMask(Mask* newMask) override {
    m_transaction.execute(new cmd::SetMask(m_document, newMask));
  }
  gfx::Point getMaskOrigin() override { return m_maskOrigin; }
  bool getFilled() override { return m_filled; }
  bool getPreviewFilled() override { return m_previewFilled; }
  int getSprayWidth() override { return m_sprayWidth; }
  int getSpraySpeed() override { return m_spraySpeed; }

  void cancel() override { m_canceled = true; }
  bool isCanceled() override { return m_canceled; }

};

tools::ToolLoop* create_tool_loop(Editor* editor, Context* context)
{
  tools::Tool* current_tool = editor->getCurrentEditorTool();
  tools::Ink* current_ink = editor->getCurrentEditorInk();
  if (!current_tool || !current_ink)
    return NULL;

  Layer* layer = editor->layer();
  if (!layer) {
    StatusBar::instance()->showTip(1000,
      "There is no active layer");
    return NULL;
  }

  // If the active layer is not visible.
  if (!layer->isVisible()) {
    StatusBar::instance()->showTip(1000,
      "Layer '%s' is hidden", layer->name().c_str());
    return NULL;
  }
  // If the active layer is read-only.
  else if (!layer->isEditable()) {
    StatusBar::instance()->showTip(1000,
      "Layer '%s' is locked", layer->name().c_str());
    return NULL;
  }

  // Get fg/bg colors
  ColorBar* colorbar = ColorBar::instance();
  app::Color fg = colorbar->getFgColor();
  app::Color bg = colorbar->getBgColor();

  ASSERT(fg.isValid());
  ASSERT(bg.isValid());

  if (!fg.isValid() || !bg.isValid()) {
    Alert::show(PACKAGE
                "<<The current selected foreground and/or background color"
                "<<is out of range. Select a valid color in the color-bar."
                "||&Close");
    return NULL;
  }

  // Create the new tool loop
  try {
    return new ToolLoopImpl(
      editor, context,
      current_tool,
      current_ink,
      editor->document(),
      !editor->isSecondaryButton() ? tools::ToolLoop::Left: tools::ToolLoop::Right,
      fg, bg);
  }
  catch (const std::exception& ex) {
    Alert::show(PACKAGE
                "<<Error drawing ink:"
                "<<%s"
                "||&Close",
                ex.what());
    return NULL;
  }
}

//////////////////////////////////////////////////////////////////////
// For preview

class PreviewToolLoopImpl : public ToolLoopBase {
  Image* m_image;

public:
  PreviewToolLoopImpl(
    Editor* editor,
    tools::Tool* tool,
    tools::Ink* ink,
    Document* document,
    const app::Color& fgColor,
    const app::Color& bgColor,
    Image* image,
    const gfx::Point& offset)
    : ToolLoopBase(editor, tool, ink, document,
                   tools::ToolLoop::Left, fgColor, bgColor)
    , m_image(image)
  {
    m_offset = offset;

    // Avoid preview for spray and flood fill like tools
    if (m_pointShape->isSpray()) {
      m_pointShape = App::instance()->getToolBox()->getPointShapeById(
        tools::WellKnownPointShapes::Brush);
    }
    else if (m_pointShape->isFloodFill()) {
      m_pointShape = App::instance()->getToolBox()->getPointShapeById(
        tools::WellKnownPointShapes::Pixel);
    }
  }

  // IToolLoop interface
  void dispose() override { }
  const Image* getSrcImage() override { return m_image; }
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

};

tools::ToolLoop* create_tool_loop_preview(
  Editor* editor, Image* image,
  const gfx::Point& offset)
{
  tools::Tool* current_tool = editor->getCurrentEditorTool();
  tools::Ink* current_ink = editor->getCurrentEditorInk();
  if (!current_tool || !current_ink)
    return NULL;

  Layer* layer = editor->layer();
  if (!layer ||
      !layer->isVisible() ||
      !layer->isEditable()) {
    return nullptr;
  }

  // Get fg/bg colors
  ColorBar* colorbar = ColorBar::instance();
  app::Color fg = colorbar->getFgColor();
  app::Color bg = colorbar->getBgColor();
  if (!fg.isValid() || !bg.isValid())
    return nullptr;

  // Create the new tool loop
  try {
    return new PreviewToolLoopImpl(
      editor,
      current_tool,
      current_ink,
      editor->document(),
      fg, bg, image, offset);
  }
  catch (const std::exception&) {
    return nullptr;
  }
}

//////////////////////////////////////////////////////////////////////

} // namespace app
