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
#include "gui/gui.h"
#include "modules/editors.h"
#include "raster/cel.h"
#include "raster/layer.h"
#include "raster/mask.h"
#include "raster/pen.h"
#include "raster/sprite.h"
#include "settings/settings.h"
#include "tools/ink.h"
#include "tools/tool.h"
#include "tools/tool_loop.h"
#include "undo/undo_history.h"
#include "util/expand_cel_canvas.h"
#include "widgets/color_bar.h"
#include "widgets/editor/editor.h"
#include "widgets/statebar.h"

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
  TiledMode m_tiled_mode;
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
  ExpandCelCanvas m_expandCelCanvas;

public:
  ToolLoopImpl(Editor* editor,
               Context* context,
               tools::Tool* tool,
               Document* document,
               Sprite* sprite,
               Layer* layer,
               tools::ToolLoop::Button button,
               const Color& primary_color,
               const Color& secondary_color)
    : m_editor(editor)
    , m_context(context)
    , m_tool(tool)
    , m_document(document)
    , m_sprite(sprite)
    , m_layer(layer)
    , m_canceled(false)
    , m_tiled_mode(m_context->getSettings()->getTiledMode())
    , m_button(button)
    , m_primary_color(color_utils::color_for_layer(primary_color, layer))
    , m_secondary_color(color_utils::color_for_layer(secondary_color, layer))
    , m_expandCelCanvas(document, sprite, layer, m_tiled_mode)
  {
    // Settings
    ISettings* settings = m_context->getSettings();

    switch (tool->getFill(m_button)) {
      case tools::FillNone:
        m_filled = false;
        break;
      case tools::FillAlways:
        m_filled = true;
        break;
      case tools::FillOptional:
        m_filled = settings->getToolSettings(m_tool)->getFilled();
        break;
    }
    m_previewFilled = settings->getToolSettings(m_tool)->getPreviewFilled();

    m_sprayWidth = settings->getToolSettings(m_tool)->getSprayWidth();
    m_spraySpeed = settings->getToolSettings(m_tool)->getSpraySpeed();

    // Create the pen
    IPenSettings* pen_settings = settings->getToolSettings(m_tool)->getPen();
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

    m_opacity = settings->getToolSettings(m_tool)->getOpacity();
    m_tolerance = settings->getToolSettings(m_tool)->getTolerance();
    m_speed.x = 0;
    m_speed.y = 0;

    m_offset.x = -x1;
    m_offset.y = -y1;

    // Set undo label for any kind of undo used in the whole loop
    if (m_document->getUndoHistory()->isEnabled()) {
      m_document->getUndoHistory()->setLabel(m_tool->getText().c_str());

      if (getInk()->isSelection() ||
          getInk()->isEyedropper() ||
          getInk()->isScrollMovement()) {
        m_document->getUndoHistory()->setModification(undo::DoesntModifyDocument);
      }
      else
        m_document->getUndoHistory()->setModification(undo::ModifyDocument);
    }
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
    }

    // If the trace was not canceled or it is not a 'paint' ink...
    if (m_canceled || !getInk()->isPaint()) {
      m_expandCelCanvas.rollback();
    }

    delete m_pen;
  }

  // IToolLoop interface
  Context* getContext() { return m_context; }
  tools::Tool* getTool() { return m_tool; }
  Pen* getPen() { return m_pen; }
  Document* getDocument() { return m_document; }
  Sprite* getSprite() { return m_sprite; }
  Layer* getLayer() { return m_layer; }
  Image* getSrcImage() { return m_expandCelCanvas.getSourceCanvas(); }
  Image* getDstImage() { return m_expandCelCanvas.getDestCanvas(); }
  bool useMask() { return m_useMask; }
  Mask* getMask() { return m_mask; }
  gfx::Point getMaskOrigin() { return m_maskOrigin; }
  ToolLoop::Button getMouseButton() { return m_button; }
  int getPrimaryColor() { return m_primary_color; }
  void setPrimaryColor(int color) { m_primary_color = color; }
  int getSecondaryColor() { return m_secondary_color; }
  void setSecondaryColor(int color) { m_secondary_color = color; }
  int getOpacity() { return m_opacity; }
  int getTolerance() { return m_tolerance; }
  TiledMode getTiledMode() { return m_tiled_mode; }
  bool getFilled() { return m_filled; }
  bool getPreviewFilled() { return m_previewFilled; }
  int getSprayWidth() { return m_sprayWidth; }
  int getSpraySpeed() { return m_spraySpeed; }
  gfx::Point getOffset() { return m_offset; }
  void setSpeed(const gfx::Point& speed) { m_speed = speed; }
  gfx::Point getSpeed() { return m_speed; }
  tools::Ink* getInk() { return m_tool->getInk(m_button); }
  tools::Controller* getController() { return m_tool->getController(m_button); }
  tools::PointShape* getPointShape() { return m_tool->getPointShape(m_button); }
  tools::Intertwine* getIntertwine() { return m_tool->getIntertwine(m_button); }
  tools::TracePolicy getTracePolicy() { return m_tool->getTracePolicy(m_button); }

  void cancel() { m_canceled = true; }
  bool isCanceled() { return m_canceled; }

  gfx::Point screenToSprite(const gfx::Point& screenPoint)
  {
    gfx::Point spritePoint;
    m_editor->screenToEditor(screenPoint.x, screenPoint.y,
                             &spritePoint.x, &spritePoint.y);
    return spritePoint;
  }

  void updateArea(const gfx::Rect& dirty_area)
  {
    int x1 = dirty_area.x-m_offset.x;
    int y1 = dirty_area.y-m_offset.y;
    int x2 = dirty_area.x-m_offset.x+dirty_area.w-1;
    int y2 = dirty_area.y-m_offset.y+dirty_area.h-1;

    acquire_bitmap(ji_screen);
    editors_draw_sprite_tiled(m_sprite, x1, y1, x2, y2);
    release_bitmap(ji_screen);
  }

  void updateStatusBar(const char* text)
  {
    app_get_statusbar()->setStatusText(0, text);
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
  ColorBar* colorbar = app_get_colorbar();
  Color fg = colorbar->getFgColor();
  Color bg = colorbar->getBgColor();

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
