/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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
#include "raster/dirty.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/mask.h"
#include "raster/pen.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "settings/settings.h"
#include "tools/ink.h"
#include "tools/tool.h"
#include "tools/tool_loop.h"
#include "undo/undo_history.h"
#include "undoers/add_cel.h"
#include "undoers/add_image.h"
#include "undoers/close_group.h"
#include "undoers/dirty_area.h"
#include "undoers/open_group.h"
#include "undoers/replace_image.h"
#include "undoers/set_cel_position.h"
#include "widgets/editor/editor.h"
#include "widgets/color_bar.h"
#include "widgets/statebar.h"

#include <allegro.h>

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
  Image* m_cel_image;
  bool m_cel_created;
  int m_old_cel_x;
  int m_old_cel_y;
  bool m_filled;
  bool m_previewFilled;
  int m_sprayWidth;
  int m_spraySpeed;
  TiledMode m_tiled_mode;
  Image* m_src_image;
  Image* m_dst_image;
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
    , m_cel(NULL)
    , m_cel_image(NULL)
    , m_cel_created(false)
    , m_canceled(false)
    , m_button(button)
    , m_primary_color(color_utils::color_for_layer(primary_color, layer))
    , m_secondary_color(color_utils::color_for_layer(secondary_color, layer))
  {
    // Settings
    ISettings* settings = m_context->getSettings();

    m_tiled_mode = settings->getTiledMode();

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
    
    // Get cel and image where we can draw

    if (m_layer->is_image()) {
      m_cel = static_cast<LayerImage*>(sprite->getCurrentLayer())->getCel(sprite->getCurrentFrame());
      if (m_cel)
	m_cel_image = sprite->getStock()->getImage(m_cel->getImage());
    }

    if (m_cel == NULL) {
      // create the image
      m_cel_image = image_new(sprite->getImgType(), sprite->getWidth(), sprite->getHeight());
      image_clear(m_cel_image,
		  m_cel_image->mask_color);

      // create the cel
      m_cel = new Cel(sprite->getCurrentFrame(), 0);
      static_cast<LayerImage*>(sprite->getCurrentLayer())->addCel(m_cel);

      m_cel_created = true;
    }

    m_old_cel_x = m_cel->getX();
    m_old_cel_y = m_cel->getY();

    // region to draw
    int x1, y1, x2, y2;

    // non-tiled
    if (m_tiled_mode == TILED_NONE) {
      x1 = MIN(m_cel->getX(), 0);
      y1 = MIN(m_cel->getY(), 0);
      x2 = MAX(m_cel->getX()+m_cel_image->w, m_sprite->getWidth());
      y2 = MAX(m_cel->getY()+m_cel_image->h, m_sprite->getHeight());
    }
    else { 			// tiled
      x1 = 0;
      y1 = 0;
      x2 = m_sprite->getWidth();
      y2 = m_sprite->getHeight();
    }

    // create two copies of the image region which we'll modify with the tool
    m_src_image = image_crop(m_cel_image,
			     x1-m_cel->getX(),
			     y1-m_cel->getY(), x2-x1, y2-y1,
			     m_cel_image->mask_color);
    m_dst_image = image_new_copy(m_src_image);

    m_useMask = m_document->isMaskVisible();

    // Selection ink
    if (getInk()->isSelection() && !m_document->isMaskVisible()) {
      Mask emptyMask;
      m_document->setMask(&emptyMask);
    }

    m_mask = m_document->getMask();
    m_maskOrigin = (!m_mask->is_empty() ? gfx::Point(m_mask->x-x1, m_mask->y-y1):
					  gfx::Point(0, 0));

    m_opacity = settings->getToolSettings(m_tool)->getOpacity();
    m_tolerance = settings->getToolSettings(m_tool)->getTolerance();
    m_speed.x = 0;
    m_speed.y = 0;

    // we have to modify the cel position because it's used in the
    // `render_sprite' routine to draw the `dst_image'
    m_cel->setPosition(x1, y1);
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
      undo::UndoHistory* undo = m_document->getUndoHistory();

      // Paint ink
      if (getInk()->isPaint()) {
	// If the size of each image is the same, we can create an
	// undo with only the differences between both images.
	if (m_cel->getX() == m_old_cel_x &&
	    m_cel->getY() == m_old_cel_y &&
	    m_cel_image->w == m_dst_image->w &&
	    m_cel_image->h == m_dst_image->h) {
	  // Was the 'cel_image' created in the start of the tool-loop?.
	  if (m_cel_created) {
	    // Then we can keep the 'cel_image'...
	  
	    // We copy the 'destination' image to the 'cel_image'.
	    image_copy(m_cel_image, m_dst_image, 0, 0);

	    // Add the 'cel_image' in the images' stock of the sprite.
	    m_cel->setImage(m_sprite->getStock()->addImage(m_cel_image));

	    // Is the undo enabled?.
	    if (undo->isEnabled()) {
	      // We can temporary remove the cel.
	      static_cast<LayerImage*>(m_sprite->getCurrentLayer())->removeCel(m_cel);

	      // We create the undo information (for the new cel_image
	      // in the stock and the new cel in the layer)...
	      undo->pushUndoer(new undoers::OpenGroup());
	      undo->pushUndoer(new undoers::AddImage(undo->getObjects(),
		  m_sprite->getStock(), m_cel->getImage()));
	      undo->pushUndoer(new undoers::AddCel(undo->getObjects(),
		  m_sprite->getCurrentLayer(), m_cel));
	      undo->pushUndoer(new undoers::CloseGroup());

	      // And finally we add the cel again in the layer.
	      static_cast<LayerImage*>(m_sprite->getCurrentLayer())->addCel(m_cel);
	    }
	  }
	  else {
	    // Undo the dirty region.
	    if (undo->isEnabled()) {
	      Dirty* dirty = new Dirty(m_cel_image, m_dst_image);
	      // TODO error handling

	      dirty->saveImagePixels(m_cel_image);
	      if (dirty != NULL)
		undo->pushUndoer(new undoers::DirtyArea(undo->getObjects(),
		    m_cel_image, dirty));

	      delete dirty;
	    }

	    // Copy the 'dst_image' to the cel_image.
	    image_copy(m_cel_image, m_dst_image, 0, 0);
	  }
	}
	// If the size of both images are different, we have to
	// replace the entire image.
	else {
	  if (undo->isEnabled()) {
	    undo->pushUndoer(new undoers::OpenGroup());

	    if (m_cel->getX() != m_old_cel_x ||
		m_cel->getY() != m_old_cel_y) {
	      int x = m_cel->getX();
	      int y = m_cel->getY();
	      m_cel->setPosition(m_old_cel_x, m_old_cel_y);

	      undo->pushUndoer(new undoers::SetCelPosition(undo->getObjects(), m_cel));

	      m_cel->setPosition(x, y);
	    }

	    undo->pushUndoer(new undoers::ReplaceImage(undo->getObjects(),
		m_sprite->getStock(), m_cel->getImage()));
	    undo->pushUndoer(new undoers::CloseGroup());
	  }

	  // Replace the image in the stock.
	  m_sprite->getStock()->replaceImage(m_cel->getImage(), m_dst_image);

	  // Destroy the old cel image.
	  image_free(m_cel_image);

	  // Now the `dst_image' is used, so we haven't to destroy it.
	  m_dst_image = NULL;
	}
      }

      // Selection ink
      if (getInk()->isSelection())
	m_document->generateMaskBoundaries();
    }

    // If the trace was not canceled or it is not a 'paint' ink...
    if (m_canceled || !getInk()->isPaint()) {
      // Here we destroy the temporary 'cel' created and restore all as it was before

      m_cel->setPosition(m_old_cel_x, m_old_cel_y);

      if (m_cel_created) {
	static_cast<LayerImage*>(m_layer)->removeCel(m_cel);
	delete m_cel;
	delete m_cel_image;
      }
    }

    delete m_src_image;
    delete m_dst_image;
    delete m_pen;
  }

  // IToolLoop interface
  Context* getContext() { return m_context; }
  tools::Tool* getTool() { return m_tool; }
  Pen* getPen() { return m_pen; }
  Document* getDocument() { return m_document; }
  Sprite* getSprite() { return m_sprite; }
  Layer* getLayer() { return m_layer; }
  Image* getSrcImage() { return m_src_image; }
  Image* getDstImage() { return m_dst_image; }
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
