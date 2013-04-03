/* ASEPRITE
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

#include "config.h"

#include "widgets/editor/tool_loop_impl.h"

#include "app.h"
#include "app/color.h"
#include "app/color_utils.h"
#include "context.h"
#include "context_access.h"
#include "raster/cel.h"
#include "raster/layer.h"
#include "raster/mask.h"
#include "raster/pen.h"
#include "raster/sprite.h"
#include "settings/document_settings.h"
#include "settings/settings.h"
#include "tools/ink.h"
#include "tools/shade_table.h"
#include "tools/shading_options.h"
#include "tools/tool.h"
#include "tools/tool_box.h"
#include "tools/tool_loop.h"
#include "ui/gui.h"
#include "undo_transaction.h"
#include "util/expand_cel_canvas.h"
#include "widgets/color_bar.h"
#include "widgets/editor/editor.h"
#include "widgets/status_bar.h"

#include <allegro.h>

using namespace ui;

class ToolLoopImpl : public tools::ToolLoop,
                     public tools::ShadingOptions
{
  Editor* m_editor;
  Context* m_context;
  tools::Tool* m_tool;
  Pen* m_pen;
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
  gfx::Point m_offset;
  gfx::Point m_speed;
  bool m_canceled;
  tools::ToolLoop::Button m_button;
  tools::Ink* m_ink;
  int m_primary_color;
  int m_secondary_color;
  UndoTransaction m_undoTransaction;
  ExpandCelCanvas m_expandCelCanvas;
  gfx::Region m_dirtyArea;
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
    , m_sprite(editor->getSprite())
    , m_layer(editor->getLayer())
    , m_frame(editor->getFrame())
    , m_canceled(false)
    , m_settings(m_context->getSettings())
    , m_docSettings(m_settings->getDocumentSettings(m_document))
    , m_toolSettings(m_settings->getToolSettings(m_tool))
    , m_button(button)
    , m_ink(getInkFromType())
    , m_primary_color(color_utils::color_for_layer(primary_color, m_layer))
    , m_secondary_color(color_utils::color_for_layer(secondary_color, m_layer))
    , m_undoTransaction(m_context,
                        m_tool->getText().c_str(),
                        ((getInk()->isSelection() ||
                          getInk()->isEyedropper() ||
                          getInk()->isScrollMovement()) ? undo::DoesntModifyDocument:
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
    m_previewFilled = m_toolSettings->getPreviewFilled();

    m_sprayWidth = m_toolSettings->getSprayWidth();
    m_spraySpeed = m_toolSettings->getSpraySpeed();

    // Create the pen
    IPenSettings* pen_settings = m_toolSettings->getPen();
    ASSERT(pen_settings != NULL);

    m_pen = new Pen(pen_settings->getType(),
                    pen_settings->getSize(),
                    pen_settings->getAngle());

    m_useMask = m_document->isMaskVisible();

    // Selection ink
    if (getInk()->isSelection() && !m_document->isMaskVisible()) {
      Mask emptyMask;
      m_document->setMask(&emptyMask);
    }

    int x1 = m_expandCelCanvas.getCel()->getX();
    int y1 = m_expandCelCanvas.getCel()->getY();

    m_mask = m_document->getMask();
    m_maskOrigin = (!m_mask->isEmpty() ? gfx::Point(m_mask->getBounds().x-x1,
                                                    m_mask->getBounds().y-y1):
                                         gfx::Point(0, 0));

    m_opacity = m_toolSettings->getOpacity();
    m_tolerance = m_toolSettings->getTolerance();
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
        m_expandCelCanvas.commit();
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

    delete m_pen;
    delete m_shadeTable;
  }

  // IToolLoop interface
  tools::Tool* getTool() OVERRIDE { return m_tool; }
  Pen* getPen() OVERRIDE { return m_pen; }
  Document* getDocument() OVERRIDE { return m_document; }
  Sprite* getSprite() OVERRIDE { return m_sprite; }
  Layer* getLayer() OVERRIDE { return m_layer; }
  Image* getSrcImage() OVERRIDE { return m_expandCelCanvas.getSourceCanvas(); }
  Image* getDstImage() OVERRIDE { return m_expandCelCanvas.getDestCanvas(); }
  RgbMap* getRgbMap() OVERRIDE { return m_sprite->getRgbMap(m_frame); }
  bool useMask() OVERRIDE { return m_useMask; }
  Mask* getMask() OVERRIDE { return m_mask; }
  gfx::Point getMaskOrigin() OVERRIDE { return m_maskOrigin; }
  ToolLoop::Button getMouseButton() OVERRIDE { return m_button; }
  int getPrimaryColor() OVERRIDE { return m_primary_color; }
  void setPrimaryColor(int color) OVERRIDE { m_primary_color = color; }
  int getSecondaryColor() OVERRIDE { return m_secondary_color; }
  void setSecondaryColor(int color) OVERRIDE { m_secondary_color = color; }
  int getOpacity() OVERRIDE { return m_opacity; }
  int getTolerance() OVERRIDE { return m_tolerance; }
  ISettings* getSettings() OVERRIDE { return m_settings; }
  IDocumentSettings* getDocumentSettings() OVERRIDE { return m_docSettings; }
  bool getFilled() OVERRIDE { return m_filled; }
  bool getPreviewFilled() OVERRIDE { return m_previewFilled; }
  int getSprayWidth() OVERRIDE { return m_sprayWidth; }
  int getSpraySpeed() OVERRIDE { return m_spraySpeed; }
  gfx::Point getOffset() OVERRIDE { return m_offset; }
  void setSpeed(const gfx::Point& speed) OVERRIDE { m_speed = speed; }
  gfx::Point getSpeed() OVERRIDE { return m_speed; }
  tools::Ink* getInk() OVERRIDE { return m_ink; }
  tools::Controller* getController() OVERRIDE { return m_tool->getController(m_button); }
  tools::PointShape* getPointShape() OVERRIDE { return m_tool->getPointShape(m_button); }
  tools::Intertwine* getIntertwine() OVERRIDE { return m_tool->getIntertwine(m_button); }
  tools::TracePolicy getTracePolicy() OVERRIDE { return m_tool->getTracePolicy(m_button); }
  tools::ShadingOptions* getShadingOptions() OVERRIDE { return this; }

  void cancel() OVERRIDE { m_canceled = true; }
  bool isCanceled() OVERRIDE { return m_canceled; }

  gfx::Point screenToSprite(const gfx::Point& screenPoint) OVERRIDE
  {
    gfx::Point spritePoint;
    m_editor->screenToEditor(screenPoint.x, screenPoint.y,
                             &spritePoint.x, &spritePoint.y);
    return spritePoint;
  }

  gfx::Region& getDirtyArea() OVERRIDE
  {
    return m_dirtyArea;
  }

  void updateDirtyArea() OVERRIDE
  {
    m_document->notifySpritePixelsModified(m_sprite, m_dirtyArea);
  }

  void updateStatusBar(const char* text) OVERRIDE
  {
    StatusBar::instance()->setStatusText(0, text);
  }

  // ShadingOptions implementation
  tools::ShadeTable8* getShadeTable() OVERRIDE
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
        id = WellKnownInks::Paint;
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

tools::ToolLoop* create_tool_loop(Editor* editor, Context* context, Message* msg)
{
  tools::Tool* current_tool = context->getSettings()->getCurrentTool();
  if (!current_tool)
    return NULL;

  Layer* layer = editor->getLayer();
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
  return
    new ToolLoopImpl(editor,
                     context,
                     current_tool,
                     editor->getDocument(),
                     msg->mouse.left ? tools::ToolLoop::Left:
                                       tools::ToolLoop::Right,
                     msg->mouse.left ? fg: bg,
                     msg->mouse.left ? bg: fg);
}
