/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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
#include "modules/editors.h"
#include "raster/cel.h"
#include "raster/layer.h"
#include "raster/mask.h"
#include "raster/pen.h"
#include "raster/sprite.h"
#include "settings/document_settings.h"
#include "settings/settings.h"
#include "tools/ink.h"
#include "tools/tool.h"
#include "tools/tool_loop.h"
#include "ui/gui.h"
#include "undo_transaction.h"
#include "util/expand_cel_canvas.h"
#include "widgets/color_bar.h"
#include "widgets/editor/editor.h"
#include "widgets/status_bar.h"

#include <allegro.h>

using namespace ui;

class ToolLoopImpl : public tools::ToolLoop
{
  Editor* m_editor;
  Context* m_context;
  tools::Tool* m_tool;
  Pen* m_pen;
  Document* m_document;
  Sprite* m_sprite;
  Layer* m_layer;
  Cel* m_cel;
  bool m_filled;
  bool m_previewFilled;
  int m_sprayWidth;
  int m_spraySpeed;
  ISettings* m_settings;
  IDocumentSettings* m_docSettings;
  bool m_useMask;
  Mask* m_mask;
  gfx::Point m_maskOrigin;
  int m_opacity;
  int m_tolerance;
  gfx::Point m_offset;
  gfx::Point m_speed;
  bool m_canceled;
  tools::ToolLoop::Button m_button;
  int m_primary_color;
  int m_secondary_color;
  UndoTransaction m_undoTransaction;
  ExpandCelCanvas m_expandCelCanvas;

public:
  ToolLoopImpl(Editor* editor,
               Context* context,
               tools::Tool* tool,
               Document* document,
               Sprite* sprite,
               Layer* layer,
               tools::ToolLoop::Button button,
               const app::Color& primary_color,
               const app::Color& secondary_color)
    : m_editor(editor)
    , m_context(context)
    , m_tool(tool)
    , m_document(document)
    , m_sprite(sprite)
    , m_layer(layer)
    , m_canceled(false)
    , m_settings(m_context->getSettings())
    , m_docSettings(m_settings->getDocumentSettings(m_document))
    , m_button(button)
    , m_primary_color(color_utils::color_for_layer(primary_color, layer))
    , m_secondary_color(color_utils::color_for_layer(secondary_color, layer))
    , m_undoTransaction(m_document,
                        m_tool->getText().c_str(),
                        ((getInk()->isSelection() ||
                          getInk()->isEyedropper() ||
                          getInk()->isScrollMovement()) ? undo::DoesntModifyDocument:
                                                          undo::ModifyDocument))
    , m_expandCelCanvas(document, sprite, layer, m_docSettings->getTiledMode(), m_undoTransaction)
  {
    IToolSettings* toolSettings = m_settings->getToolSettings(m_tool);

    // Settings
    switch (tool->getFill(m_button)) {
      case tools::FillNone:
        m_filled = false;
        break;
      case tools::FillAlways:
        m_filled = true;
        break;
      case tools::FillOptional:
        m_filled = toolSettings->getFilled();
        break;
    }
    m_previewFilled = toolSettings->getPreviewFilled();

    m_sprayWidth = toolSettings->getSprayWidth();
    m_spraySpeed = toolSettings->getSpraySpeed();

    // Create the pen
    IPenSettings* pen_settings = toolSettings->getPen();
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

    m_opacity = toolSettings->getOpacity();
    m_tolerance = toolSettings->getTolerance();
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
  }

  // IToolLoop interface
  tools::Tool* getTool() OVERRIDE { return m_tool; }
  Pen* getPen() OVERRIDE { return m_pen; }
  Document* getDocument() OVERRIDE { return m_document; }
  Sprite* getSprite() OVERRIDE { return m_sprite; }
  Layer* getLayer() OVERRIDE { return m_layer; }
  Image* getSrcImage() OVERRIDE { return m_expandCelCanvas.getSourceCanvas(); }
  Image* getDstImage() OVERRIDE { return m_expandCelCanvas.getDestCanvas(); }
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
  tools::Ink* getInk() OVERRIDE { return m_tool->getInk(m_button); }
  tools::Controller* getController() OVERRIDE { return m_tool->getController(m_button); }
  tools::PointShape* getPointShape() OVERRIDE { return m_tool->getPointShape(m_button); }
  tools::Intertwine* getIntertwine() OVERRIDE { return m_tool->getIntertwine(m_button); }
  tools::TracePolicy getTracePolicy() OVERRIDE { return m_tool->getTracePolicy(m_button); }

  void cancel() OVERRIDE { m_canceled = true; }
  bool isCanceled() OVERRIDE { return m_canceled; }

  gfx::Point screenToSprite(const gfx::Point& screenPoint) OVERRIDE
  {
    gfx::Point spritePoint;
    m_editor->screenToEditor(screenPoint.x, screenPoint.y,
                             &spritePoint.x, &spritePoint.y);
    return spritePoint;
  }

  void updateArea(const gfx::Rect& dirty_area) OVERRIDE
  {
    int x1 = dirty_area.x-m_offset.x;
    int y1 = dirty_area.y-m_offset.y;
    int x2 = dirty_area.x-m_offset.x+dirty_area.w-1;
    int y2 = dirty_area.y-m_offset.y+dirty_area.h-1;

    acquire_bitmap(ji_screen);
    editors_draw_sprite_tiled(m_sprite, x1, y1, x2, y2);
    release_bitmap(ji_screen);
  }

  void updateStatusBar(const char* text) OVERRIDE
  {
    StatusBar::instance()->setStatusText(0, text);
  }
};

tools::ToolLoop* create_tool_loop(Editor* editor, Context* context, Message* msg)
{
  tools::Tool* current_tool = context->getSettings()->getCurrentTool();
  if (!current_tool)
    return NULL;

  Sprite* sprite = editor->getSprite();
  Layer* layer = sprite->getCurrentLayer();

  if (!layer) {
    Alert::show(PACKAGE "<<The current sprite does not have any layer.||&Close");
    return NULL;
  }

  // If the active layer is not visible.
  if (!layer->is_readable()) {
    Alert::show(PACKAGE
                "<<The current layer is hidden,"
                "<<make it visible and try again"
                "||&Close");
    return NULL;
  }
  // If the active layer is read-only.
  else if (!layer->is_writable()) {
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
                     sprite, layer,
                     msg->mouse.left ? tools::ToolLoop::Left:
                                       tools::ToolLoop::Right,
                     msg->mouse.left ? fg: bg,
                     msg->mouse.left ? bg: fg);
}
