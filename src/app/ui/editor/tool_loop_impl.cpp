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
#include "app/pref/preferences.h"
#include "app/settings/settings.h"
#include "app/tools/controller.h"
#include "app/tools/ink.h"
#include "app/tools/shade_table.h"
#include "app/tools/shading_options.h"
#include "app/tools/tool.h"
#include "app/tools/tool_box.h"
#include "app/tools/tool_loop.h"
#include "app/transaction.h"
#include "app/ui/color_bar.h"
#include "app/ui/editor/editor.h"
#include "app/ui/status_bar.h"
#include "app/util/expand_cel_canvas.h"
#include "doc/brush.h"
#include "doc/cel.h"
#include "doc/layer.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "ui/ui.h"

namespace app {

using namespace ui;

class ToolLoopImpl : public tools::ToolLoop,
                     public tools::ShadingOptions {
  Editor* m_editor;
  Context* m_context;
  tools::Tool* m_tool;
  Brush* m_brush;
  Document* m_document;
  Sprite* m_sprite;
  Layer* m_layer;
  frame_t m_frame;
  bool m_filled;
  bool m_previewFilled;
  int m_sprayWidth;
  int m_spraySpeed;
  ISettings* m_settings;
  DocumentPreferences& m_docPref;
  IToolSettings* m_toolSettings;
  bool m_useMask;
  Mask* m_mask;
  gfx::Point m_maskOrigin;
  int m_opacity;
  int m_tolerance;
  bool m_contiguous;
  gfx::Point m_offset;
  gfx::Point m_speed;
  bool m_canceled;
  tools::ToolLoop::Button m_button;
  tools::Ink* m_ink;
  int m_primary_color;
  int m_secondary_color;
  Transaction m_transaction;
  ExpandCelCanvas m_expandCelCanvas;
  gfx::Region m_dirtyArea;
  tools::ShadeTable8* m_shadeTable;

public:
  ToolLoopImpl(Editor* editor,
               Context* context,
               tools::Tool* tool,
               tools::Ink* ink,
               Document* document,
               tools::ToolLoop::Button button,
               const app::Color& primary_color,
               const app::Color& secondary_color)
    : m_editor(editor)
    , m_context(context)
    , m_tool(tool)
    , m_document(document)
    , m_sprite(editor->sprite())
    , m_layer(editor->layer())
    , m_frame(editor->frame())
    , m_settings(m_context->settings())
    , m_docPref(App::instance()->preferences().document(m_document))
    , m_toolSettings(m_settings->getToolSettings(m_tool))
    , m_canceled(false)
    , m_button(button)
    , m_ink(ink)
    , m_primary_color(color_utils::color_for_layer(primary_color, m_layer))
    , m_secondary_color(color_utils::color_for_layer(secondary_color, m_layer))
    , m_transaction(m_context,
                    m_tool->getText().c_str(),
                    ((getInk()->isSelection() ||
                      getInk()->isEyedropper() ||
                      getInk()->isScrollMovement() ||
                      getInk()->isSlice() ||
                      getInk()->isZoom()) ? DoesntModifyDocument:
                                            ModifyDocument))
    , m_expandCelCanvas(editor->getDocumentLocation(),
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
    , m_shadeTable(NULL)
  {
    // Settings
    switch (tool->getFill(m_button)) {
      case tools::FillNone:
        m_filled = false;
        break;
      case tools::FillAlways:
        m_filled = true;
        break;
      case tools::FillOptional:
        m_filled = m_toolSettings->getFilled();
        break;
    }

    m_previewFilled = m_toolSettings->getPreviewFilled();

    m_sprayWidth = m_toolSettings->getSprayWidth();
    m_spraySpeed = m_toolSettings->getSpraySpeed();

    // Create the brush
    IBrushSettings* brush_settings = m_toolSettings->getBrush();
    ASSERT(brush_settings != NULL);

    m_brush = new Brush(
      brush_settings->getType(),
      brush_settings->getSize(),
      brush_settings->getAngle());

    if (m_ink->isSelection())
      m_useMask = false;
    else
      m_useMask = m_document->isMaskVisible();

    // Start with an empty mask if the user is selecting with "default selection mode"
    if (getInk()->isSelection() &&
        (!m_document->isMaskVisible() ||
          getSelectionMode() == kDefaultSelectionMode)) {
      Mask emptyMask;
      m_transaction.execute(new cmd::SetMask(m_document, &emptyMask));
    }

    int x1 = m_expandCelCanvas.getCel()->x();
    int y1 = m_expandCelCanvas.getCel()->y();

    m_mask = m_document->mask();
    m_maskOrigin = (!m_mask->isEmpty() ? gfx::Point(m_mask->bounds().x-x1,
                                                    m_mask->bounds().y-y1):
                                         gfx::Point(0, 0));

    m_opacity = m_toolSettings->getOpacity();
    m_tolerance = m_toolSettings->getTolerance();
    m_contiguous = m_toolSettings->getContiguous();
    m_speed.x = 0;
    m_speed.y = 0;

    m_offset.x = -x1;
    m_offset.y = -y1;
  }

  ~ToolLoopImpl()
  {
    bool redraw = false;

    if (!m_canceled) {
      // Paint ink
      if (getInk()->isPaint()) {
        for (int i=0; ; ++i) {
          // TODO add a "wait_n_seconds" parameter to ContextReader/Writer
          try {
            ContextReader reader(m_context);
            ContextWriter writer(reader);
            m_expandCelCanvas.commit();
            break;
          }
          catch (const LockedDocumentException& ex) {
            if (i == 10)
              Console::showException(ex);
            else
              base::this_thread::sleep_for(0.10);
          }
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
      for (int i=0; ; ++i) {
        try {
          ContextReader reader(m_context);
          ContextWriter writer(reader);
          m_expandCelCanvas.rollback();
          break;
        }
        catch (const LockedDocumentException& ex) {
          if (i == 10)
            Console::showException(ex);
          else
            base::this_thread::sleep_for(0.10);
        }
      }
    }

    delete m_brush;
    delete m_shadeTable;

    if (redraw)
      update_screen_for_document(m_document);
  }

  // IToolLoop interface
  tools::Tool* getTool() override { return m_tool; }
  Brush* getBrush() override { return m_brush; }
  Document* getDocument() override { return m_document; }
  Sprite* sprite() override { return m_sprite; }
  Layer* getLayer() override { return m_layer; }
  frame_t getFrame() override { return m_frame; }

  const Image* getSrcImage() override { return m_expandCelCanvas.getSourceCanvas(); }
  Image* getDstImage() override { return m_expandCelCanvas.getDestCanvas(); }
  void validateSrcImage(const gfx::Region& rgn) override {
    return m_expandCelCanvas.validateSourceCanvas(rgn);
  }
  void validateDstImage(const gfx::Region& rgn) override {
    return m_expandCelCanvas.validateDestCanvas(rgn);
  }
  void invalidateDstImage() override {
    return m_expandCelCanvas.invalidateDestCanvas();
  }
  void invalidateDstImage(const gfx::Region& rgn) override {
    return m_expandCelCanvas.invalidateDestCanvas(rgn);
  }
  void copyValidDstToSrcImage(const gfx::Region& rgn) override {
    return m_expandCelCanvas.copyValidDestToSourceCanvas(rgn);
  }

  RgbMap* getRgbMap() override { return m_sprite->rgbMap(m_frame); }
  bool useMask() override { return m_useMask; }
  Mask* getMask() override { return m_mask; }
  void setMask(Mask* newMask) override {
    m_transaction.execute(new cmd::SetMask(m_document, newMask));
  }
  gfx::Point getMaskOrigin() override { return m_maskOrigin; }
  const render::Zoom& zoom() override { return m_editor->zoom(); }
  ToolLoop::Button getMouseButton() override { return m_button; }
  int getPrimaryColor() override { return m_primary_color; }
  void setPrimaryColor(int color) override { m_primary_color = color; }
  int getSecondaryColor() override { return m_secondary_color; }
  void setSecondaryColor(int color) override { m_secondary_color = color; }
  int getOpacity() override { return m_opacity; }
  int getTolerance() override { return m_tolerance; }
  bool getContiguous() override { return m_contiguous; }
  SelectionMode getSelectionMode() override { return m_editor->getSelectionMode(); }
  ISettings* settings() override { return m_settings; }
  filters::TiledMode getTiledMode() override { return m_docPref.tiled.mode(); }
  bool getGridVisible() override { return m_docPref.grid.visible(); }
  bool getSnapToGrid() override { return m_docPref.grid.snap(); }
  gfx::Rect getGridBounds() override { return m_docPref.grid.bounds(); }
  bool getFilled() override { return m_filled; }
  bool getPreviewFilled() override { return m_previewFilled; }
  int getSprayWidth() override { return m_sprayWidth; }
  int getSpraySpeed() override { return m_spraySpeed; }
  gfx::Point getOffset() override { return m_offset; }
  void setSpeed(const gfx::Point& speed) override { m_speed = speed; }
  gfx::Point getSpeed() override { return m_speed; }
  tools::Ink* getInk() override { return m_ink; }
  tools::Controller* getController() override { return m_tool->getController(m_button); }
  tools::PointShape* getPointShape() override { return m_tool->getPointShape(m_button); }
  tools::Intertwine* getIntertwine() override { return m_tool->getIntertwine(m_button); }
  tools::TracePolicy getTracePolicy() override { return m_tool->getTracePolicy(m_button); }
  tools::ShadingOptions* getShadingOptions() override { return this; }

  void cancel() override { m_canceled = true; }
  bool isCanceled() override { return m_canceled; }

  gfx::Point screenToSprite(const gfx::Point& screenPoint) override
  {
    return m_editor->screenToEditor(screenPoint);
  }

  gfx::Region& getDirtyArea() override
  {
    return m_dirtyArea;
  }

  void updateDirtyArea() override
  {
    m_editor->hideDrawingCursor();
    m_document->notifySpritePixelsModified(m_sprite, m_dirtyArea);
    m_editor->showDrawingCursor();
  }

  void updateStatusBar(const char* text) override
  {
    StatusBar::instance()->setStatusText(0, text);
  }

  // ShadingOptions implementation
  tools::ShadeTable8* getShadeTable() override
  {
    if (m_shadeTable == NULL) {
      app::ColorSwatches* colorSwatches = m_settings->getColorSwatches();
      ASSERT(colorSwatches != NULL);
      m_shadeTable = new tools::ShadeTable8(*colorSwatches,
                                            tools::kRotateShadingMode);
    }
    return m_shadeTable;
  }

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

  if (!fg.isValid() || !bg.isValid()) {
    Alert::show(PACKAGE
                "<<The current selected foreground and/or background color"
                "<<is out of range. Select valid colors in the color-bar."
                "||&Close");
    return NULL;
  }

  // Create the new tool loop
  try {
    return new ToolLoopImpl(editor, context,
      current_tool,
      current_ink,
      editor->document(),
      !editor->isSecondaryButton() ? tools::ToolLoop::Left: tools::ToolLoop::Right,
      !editor->isSecondaryButton() ? fg: bg,
      !editor->isSecondaryButton() ? bg: fg);
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

} // namespace app
