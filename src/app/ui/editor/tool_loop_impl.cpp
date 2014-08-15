/* Aseprite
 * Copyright (C) 2001-2014  David Capello
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

#include "app/ui/editor/tool_loop_impl.h"

#include "app/app.h"
#include "app/color.h"
#include "app/color_utils.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/settings/document_settings.h"
#include "app/settings/settings.h"
#include "app/tools/ink.h"
#include "app/tools/shade_table.h"
#include "app/tools/shading_options.h"
#include "app/tools/tool.h"
#include "app/tools/tool_box.h"
#include "app/tools/tool_loop.h"
#include "app/ui/color_bar.h"
#include "app/ui/editor/editor.h"
#include "app/ui/status_bar.h"
#include "app/undo_transaction.h"
#include "app/util/expand_cel_canvas.h"
#include "raster/brush.h"
#include "raster/cel.h"
#include "raster/layer.h"
#include "raster/mask.h"
#include "raster/sprite.h"
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
  FrameNumber m_frame;
  Cel* m_cel;
  bool m_filled;
  bool m_previewFilled;
  int m_sprayWidth;
  int m_spraySpeed;
  ISettings* m_settings;
  IDocumentSettings* m_docSettings;
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
  SelectionMode m_selectionMode;
  UndoTransaction m_undoTransaction;
  ExpandCelCanvas m_expandCelCanvas;
  gfx::Region m_dirtyArea;
  gfx::Rect m_dirtyBounds;
  tools::ShadeTable8* m_shadeTable;

public:
  ToolLoopImpl(Editor* editor,
               Context* context,
               tools::Tool* tool,
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
    , m_canceled(false)
    , m_settings(m_context->settings())
    , m_docSettings(m_settings->getDocumentSettings(m_document))
    , m_toolSettings(m_settings->getToolSettings(m_tool))
    , m_button(button)
    , m_ink(getInkFromType())
    , m_primary_color(color_utils::color_for_layer(primary_color, m_layer))
    , m_secondary_color(color_utils::color_for_layer(secondary_color, m_layer))
    , m_selectionMode(m_settings->selection()->getSelectionMode())
    , m_undoTransaction(m_context,
                        m_tool->getText().c_str(),
                        ((getInk()->isSelection() ||
                          getInk()->isEyedropper() ||
                          getInk()->isScrollMovement() ||
                          getInk()->isSlice() ||
                          getInk()->isZoom()) ? undo::DoesntModifyDocument:
                                                undo::ModifyDocument))
    , m_expandCelCanvas(m_context, m_docSettings->getTiledMode(), m_undoTransaction)
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

    // Right-click subtract selection always.
    if (m_button == 1)
      m_selectionMode = kSubtractSelectionMode;

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

    m_useMask = m_document->isMaskVisible();

    // Selection ink
    if (getInk()->isSelection() &&
        (!m_document->isMaskVisible() ||
          m_selectionMode == kDefaultSelectionMode)) {
      Mask emptyMask;
      m_document->setMask(&emptyMask);
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
    if (!m_canceled) {
      // Paint ink
      if (getInk()->isPaint()) {
        m_expandCelCanvas.commit(m_dirtyBounds);
      }
      // Selection ink
      else if (getInk()->isSelection()) {
        m_document->generateMaskBoundaries();
      }

      m_undoTransaction.commit();
    }

    // If the trace was not canceled or it is not a 'paint' ink...
    if (m_canceled || !getInk()->isPaint()) {
      m_expandCelCanvas.rollback();
    }

    delete m_brush;
    delete m_shadeTable;
  }

  // IToolLoop interface
  tools::Tool* getTool() override { return m_tool; }
  Brush* getBrush() override { return m_brush; }
  Document* getDocument() override { return m_document; }
  Sprite* sprite() override { return m_sprite; }
  Layer* getLayer() override { return m_layer; }
  Image* getSrcImage() override { return m_expandCelCanvas.getSourceCanvas(); }
  Image* getDstImage() override { return m_expandCelCanvas.getDestCanvas(); }
  RgbMap* getRgbMap() override { return m_sprite->getRgbMap(m_frame); }
  bool useMask() override { return m_useMask; }
  Mask* getMask() override { return m_mask; }
  gfx::Point getMaskOrigin() override { return m_maskOrigin; }
  ToolLoop::Button getMouseButton() override { return m_button; }
  int getPrimaryColor() override { return m_primary_color; }
  void setPrimaryColor(int color) override { m_primary_color = color; }
  int getSecondaryColor() override { return m_secondary_color; }
  void setSecondaryColor(int color) override { m_secondary_color = color; }
  int getOpacity() override { return m_opacity; }
  int getTolerance() override { return m_tolerance; }
  bool getContiguous() override { return m_contiguous; }
  SelectionMode getSelectionMode() override { return m_selectionMode; }
  ISettings* settings() override { return m_settings; }
  IDocumentSettings* getDocumentSettings() override { return m_docSettings; }
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
    gfx::Point spritePoint;
    m_editor->screenToEditor(screenPoint.x, screenPoint.y,
                             &spritePoint.x, &spritePoint.y);
    return spritePoint;
  }

  gfx::Region& getDirtyArea() override
  {
    return m_dirtyArea;
  }

  void updateDirtyArea() override
  {
    m_dirtyBounds = m_dirtyBounds.createUnion(m_dirtyArea.getBounds());
    m_document->notifySpritePixelsModified(m_sprite, m_dirtyArea);
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

private:
  tools::Ink* getInkFromType()
  {
    using namespace tools;

    InkType inkType = m_toolSettings->getInkType();
    if (inkType == kDefaultInk)
      return m_tool->getInk(m_button);

    const char* id = WellKnownInks::Paint;
    switch (inkType) {
      case kOpaqueInk:
        id = WellKnownInks::PaintOpaque;
        break;
      case kSetAlphaInk:
        id = WellKnownInks::PaintSetAlpha;
        break;
      case kLockAlphaInk:
        id = WellKnownInks::PaintLockAlpha;
        break;
      case kMergeInk:
        id = WellKnownInks::Paint;
        break;
      case kShadingInk:
        id = WellKnownInks::Shading;
        break;
      case kReplaceInk:
        if (m_button == ToolLoop::Left)
          id = WellKnownInks::ReplaceBgWithFg;
        else
          id = WellKnownInks::ReplaceFgWithBg;
        break;
      case kEraseInk:
        id = WellKnownInks::Eraser;
        break;
      case kSelectionInk:
        id = WellKnownInks::Selection;
        break;
      case kBlurInk:
        id = WellKnownInks::Blur;
        break;
      case kJumbleInk:
        id = WellKnownInks::Jumble;
        break;
    }

    return App::instance()->getToolBox()->getInkById(id);
  }

};

tools::ToolLoop* create_tool_loop(Editor* editor, Context* context, MouseMessage* msg)
{
  tools::Tool* current_tool = context->settings()->getCurrentTool();
  if (!current_tool)
    return NULL;

  Layer* layer = editor->layer();
  if (!layer) {
    Alert::show(PACKAGE "<<The current sprite does not have any layer.||&Close");
    return NULL;
  }

  // If the active layer is not visible.
  if (!layer->isReadable()) {
    Alert::show(PACKAGE
                "<<The current layer is hidden,"
                "<<make it visible and try again"
                "||&Close");
    return NULL;
  }
  // If the active layer is read-only.
  else if (!layer->isWritable()) {
    Alert::show(PACKAGE
                "<<The current layer is locked,"
                "<<unlock it and try again"
                "||&Close");
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
  try
  {
    return new ToolLoopImpl(editor,
                            context,
                            current_tool,
                            editor->document(),
                            msg->left() ? tools::ToolLoop::Left:
                                          tools::ToolLoop::Right,
                            msg->left() ? fg: bg,
                            msg->left() ? bg: fg);
  }
  catch (const std::exception& ex)
  {
    Alert::show(PACKAGE
                "<<Error drawing ink:"
                "<<%s"
                "||&Close",
                ex.what());
    return NULL;
  }
}

} // namespace app
