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

#include "util/celmove.h"

#include "app.h"
#include "app/color.h"
#include "console.h"
#include "document_wrappers.h"
#include "modules/gui.h"
#include "raster/blend.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "undo_transaction.h"
#include "undoers/add_cel.h"
#include "undoers/add_image.h"
#include "undoers/remove_cel.h"
#include "undoers/remove_image.h"
#include "undoers/replace_image.h"
#include "undoers/set_cel_frame.h"
#include "undoers/set_cel_opacity.h"
#include "undoers/set_cel_position.h"
#include "undoers/set_current_frame.h"
#include "undoers/set_current_layer.h"

/* these variables indicate what cel to move (and the sprite's
   frame indicates where to move it) */
static Layer* src_layer = NULL; // TODO warning not thread safe
static Layer* dst_layer = NULL;
static FrameNumber src_frame = FrameNumber(0);
static FrameNumber dst_frame = FrameNumber(0);

static void remove_cel(Sprite* sprite, UndoTransaction& undo, LayerImage *layer, Cel *cel);

void set_frame_to_handle(Layer *_src_layer, FrameNumber _src_frame,
                         Layer *_dst_layer, FrameNumber _dst_frame)
{
  src_layer = _src_layer;
  src_frame = _src_frame;
  dst_layer = _dst_layer;
  dst_frame = _dst_frame;
}

void move_cel(DocumentWriter& document)
{
  Sprite* sprite = document->getSprite();
  Cel *src_cel, *dst_cel;

  ASSERT(src_layer != NULL);
  ASSERT(dst_layer != NULL);
  ASSERT(src_frame >= 0 && src_frame < sprite->getTotalFrames());
  ASSERT(dst_frame >= 0 && dst_frame < sprite->getTotalFrames());

  if (src_layer->isBackground()) {
    copy_cel(document);
    return;
  }

  src_cel = static_cast<LayerImage*>(src_layer)->getCel(src_frame);
  dst_cel = static_cast<LayerImage*>(dst_layer)->getCel(dst_frame);

  UndoTransaction undo(document, "Move Cel", undo::ModifyDocument);
  if (undo.isEnabled()) {
    undo.pushUndoer(new undoers::SetCurrentLayer(undo.getObjects(), sprite));
    undo.pushUndoer(new undoers::SetCurrentFrame(undo.getObjects(), sprite));
  }

  sprite->setCurrentLayer(dst_layer);
  sprite->setCurrentFrame(dst_frame);

  /* remove the 'dst_cel' (if it exists) because it must be
     replaced with 'src_cel' */
  if ((dst_cel != NULL) && (!dst_layer->isBackground() || src_cel != NULL))
    remove_cel(sprite, undo, static_cast<LayerImage*>(dst_layer), dst_cel);

  /* move the cel in the same layer */
  if (src_cel != NULL) {
    if (src_layer == dst_layer) {
      if (undo.isEnabled())
        undo.pushUndoer(new undoers::SetCelFrame(undo.getObjects(), src_cel));

      src_cel->setFrame(dst_frame);
    }
    /* move the cel in different layers */
    else {
      if (undo.isEnabled())
        undo.pushUndoer(new undoers::RemoveCel(undo.getObjects(), src_layer, src_cel));
      static_cast<LayerImage*>(src_layer)->removeCel(src_cel);

      src_cel->setFrame(dst_frame);

      /* if we are moving a cel from a transparent layer to the
         background layer, we have to clear the background of the
         image */
      if (!src_layer->isBackground() &&
          dst_layer->isBackground()) {
        Image *src_image = sprite->getStock()->getImage(src_cel->getImage());
        Image *dst_image = image_crop(src_image,
                                      -src_cel->getX(),
                                      -src_cel->getY(),
                                      sprite->getWidth(),
                                      sprite->getHeight(), 0);

        if (undo.isEnabled()) {
          undo.pushUndoer(new undoers::ReplaceImage(undo.getObjects(),
              sprite->getStock(), src_cel->getImage()));
          undo.pushUndoer(new undoers::SetCelPosition(undo.getObjects(), src_cel));
          undo.pushUndoer(new undoers::SetCelOpacity(undo.getObjects(), src_cel));
        }

        image_clear(dst_image, app_get_color_to_clear_layer(dst_layer));
        image_merge(dst_image, src_image, src_cel->getX(), src_cel->getY(), 255, BLEND_MODE_NORMAL);

        src_cel->setPosition(0, 0);
        src_cel->setOpacity(255);

        sprite->getStock()->replaceImage(src_cel->getImage(), dst_image);
        image_free(src_image);
      }

      if (undo.isEnabled())
        undo.pushUndoer(new undoers::AddCel(undo.getObjects(), dst_layer, src_cel));

      static_cast<LayerImage*>(dst_layer)->addCel(src_cel);
    }
  }

  undo.commit();

  set_frame_to_handle(NULL, FrameNumber(0), NULL, FrameNumber(0));
}

void copy_cel(DocumentWriter& document)
{
  Sprite* sprite = document->getSprite();
  UndoTransaction undo(document, "Move Cel", undo::ModifyDocument);
  Cel *src_cel, *dst_cel;

  ASSERT(src_layer != NULL);
  ASSERT(dst_layer != NULL);
  ASSERT(src_frame >= 0 && src_frame < sprite->getTotalFrames());
  ASSERT(dst_frame >= 0 && dst_frame < sprite->getTotalFrames());

  src_cel = static_cast<LayerImage*>(src_layer)->getCel(src_frame);
  dst_cel = static_cast<LayerImage*>(dst_layer)->getCel(dst_frame);

  if (undo.isEnabled()) {
    undo.pushUndoer(new undoers::SetCurrentLayer(undo.getObjects(), sprite));
    undo.pushUndoer(new undoers::SetCurrentFrame(undo.getObjects(), sprite));
  }

  sprite->setCurrentLayer(dst_layer);
  sprite->setCurrentFrame(dst_frame);

  /* remove the 'dst_cel' (if it exists) because it must be
     replaced with 'src_cel' */
  if ((dst_cel != NULL) && (!dst_layer->isBackground() || src_cel != NULL))
    remove_cel(sprite, undo, static_cast<LayerImage*>(dst_layer), dst_cel);

  /* move the cel in the same layer */
  if (src_cel != NULL) {
    Image *src_image = sprite->getStock()->getImage(src_cel->getImage());
    Image *dst_image;
    int image_index;
    int dst_cel_x;
    int dst_cel_y;
    int dst_cel_opacity;

    /* if we are moving a cel from a transparent layer to the
       background layer, we have to clear the background of the
       image */
    if (!src_layer->isBackground() &&
        dst_layer->isBackground()) {
      dst_image = image_crop(src_image,
                             -src_cel->getX(),
                             -src_cel->getY(),
                             sprite->getWidth(),
                             sprite->getHeight(), 0);

      image_clear(dst_image, app_get_color_to_clear_layer(dst_layer));
      image_merge(dst_image, src_image, src_cel->getX(), src_cel->getY(), 255, BLEND_MODE_NORMAL);

      dst_cel_x = 0;
      dst_cel_y = 0;
      dst_cel_opacity = 255;
    }
    else {
      dst_image = Image::createCopy(src_image);
      dst_cel_x = src_cel->getX();
      dst_cel_y = src_cel->getY();
      dst_cel_opacity = src_cel->getOpacity();
    }

    /* add the image in the stock */
    image_index = sprite->getStock()->addImage(dst_image);
    if (undo.isEnabled())
      undo.pushUndoer(new undoers::AddImage(undo.getObjects(),
          sprite->getStock(), image_index));

    /* create the new cel */
    dst_cel = new Cel(dst_frame, image_index);
    dst_cel->setPosition(dst_cel_x, dst_cel_y);
    dst_cel->setOpacity(dst_cel_opacity);

    if (undo.isEnabled())
      undo.pushUndoer(new undoers::AddCel(undo.getObjects(), dst_layer, dst_cel));

    static_cast<LayerImage*>(dst_layer)->addCel(dst_cel);
  }

  undo.commit();

  set_frame_to_handle(NULL, FrameNumber(0), NULL, FrameNumber(0));
}

static void remove_cel(Sprite* sprite, UndoTransaction& undo, LayerImage *layer, Cel *cel)
{
  Image *image;
  Cel *it;
  bool used;

  if (sprite != NULL && layer->isImage() && cel != NULL) {
    /* find if the image that use the cel to remove, is used by
       another cels */
    used = false;
    for (FrameNumber frame(0); frame<sprite->getTotalFrames(); ++frame) {
      it = layer->getCel(frame);
      if (it != NULL && it != cel && it->getImage() == cel->getImage()) {
        used = true;
        break;
      }
    }

    if (!used) {
      // If the image is only used by this cel, we can remove the
      // image from the stock.
      image = sprite->getStock()->getImage(cel->getImage());

      if (undo.isEnabled())
        undo.pushUndoer(new undoers::RemoveImage(undo.getObjects(),
            sprite->getStock(), cel->getImage()));

      sprite->getStock()->removeImage(image);
      image_free(image);
    }

    if (undo.isEnabled()) {
      undo.pushUndoer(new undoers::RemoveCel(undo.getObjects(), layer, cel));
    }

    // Remove the cel
    layer->removeCel(cel);
    delete cel;
  }
}
