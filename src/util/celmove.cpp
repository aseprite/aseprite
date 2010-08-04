/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include <jinete/jlist.h>

#include "sprite_wrappers.h"
#include "console.h"
#include "app.h"
#include "core/color.h"
#include "core/core.h"
#include "modules/gui.h"
#include "raster/blend.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "raster/undo.h"
#include "util/celmove.h"

/* these variables indicate what cel to move (and the sprite's
   frame indicates where to move it) */
static Layer* src_layer = NULL;	// TODO warning not thread safe
static Layer* dst_layer = NULL;
static int src_frame = 0;
static int dst_frame = 0;

static void remove_cel(Sprite* sprite, LayerImage* layer, Cel* cel);

void set_frame_to_handle(Layer *_src_layer, int _src_frame,
			 Layer *_dst_layer, int _dst_frame)
{
  src_layer = _src_layer;
  src_frame = _src_frame;
  dst_layer = _dst_layer;
  dst_frame = _dst_frame;
}

void move_cel(SpriteWriter& sprite)
{
  Cel *src_cel, *dst_cel;

  ASSERT(src_layer != NULL);
  ASSERT(dst_layer != NULL);
  ASSERT(src_frame >= 0 && src_frame < sprite->getTotalFrames());
  ASSERT(dst_frame >= 0 && dst_frame < sprite->getTotalFrames());

  if (src_layer->is_background()) {
    copy_cel(sprite);
    return;
  }

  src_cel = static_cast<LayerImage*>(src_layer)->get_cel(src_frame);
  dst_cel = static_cast<LayerImage*>(dst_layer)->get_cel(dst_frame);

  if (undo_is_enabled(sprite->getUndo())) {
    undo_set_label(sprite->getUndo(), "Move Cel");
    undo_open(sprite->getUndo());

    undo_set_layer(sprite->getUndo(), sprite);
    undo_set_frame(sprite->getUndo(), sprite);
  }

  sprite->setCurrentLayer(dst_layer);
  sprite->setCurrentFrame(dst_frame);

  /* remove the 'dst_cel' (if it exists) because it must be
     replaced with 'src_cel' */
  if ((dst_cel != NULL) && (!dst_layer->is_background() || src_cel != NULL))
    remove_cel(sprite, static_cast<LayerImage*>(dst_layer), dst_cel);

  /* move the cel in the same layer */
  if (src_cel != NULL) {
    if (src_layer == dst_layer) {
      if (undo_is_enabled(sprite->getUndo()))
	undo_int(sprite->getUndo(), (GfxObj *)src_cel, &src_cel->frame);

      src_cel->frame = dst_frame;
    }
    /* move the cel in different layers */
    else {
      if (undo_is_enabled(sprite->getUndo()))
	undo_remove_cel(sprite->getUndo(), src_layer, src_cel);
      static_cast<LayerImage*>(src_layer)->remove_cel(src_cel);

      src_cel->frame = dst_frame;

      /* if we are moving a cel from a transparent layer to the
	 background layer, we have to clear the background of the
	 image */
      if (!src_layer->is_background() &&
	  dst_layer->is_background()) {
	Image *src_image = stock_get_image(sprite->getStock(), src_cel->image);
	Image *dst_image = image_crop(src_image,
				      -src_cel->x,
				      -src_cel->y,
				      sprite->getWidth(),
				      sprite->getHeight(), 0);

	if (undo_is_enabled(sprite->getUndo())) {
	  undo_replace_image(sprite->getUndo(), sprite->getStock(), src_cel->image);
	  undo_int(sprite->getUndo(), (GfxObj *)src_cel, &src_cel->x);
	  undo_int(sprite->getUndo(), (GfxObj *)src_cel, &src_cel->y);
	  undo_int(sprite->getUndo(), (GfxObj *)src_cel, &src_cel->opacity);
	}

	image_clear(dst_image, app_get_color_to_clear_layer(dst_layer));
	image_merge(dst_image, src_image, src_cel->x, src_cel->y, 255, BLEND_MODE_NORMAL);

	src_cel->x = 0;
	src_cel->y = 0;
	src_cel->opacity = 255;

	stock_replace_image(sprite->getStock(), src_cel->image, dst_image);
	image_free(src_image);
      }
      
      if (undo_is_enabled(sprite->getUndo()))
	undo_add_cel(sprite->getUndo(), dst_layer, src_cel);

      static_cast<LayerImage*>(dst_layer)->add_cel(src_cel);
    }
  }

  if (undo_is_enabled(sprite->getUndo()))
    undo_close(sprite->getUndo());

  set_frame_to_handle(NULL, 0, NULL, 0);
}

void copy_cel(SpriteWriter& sprite)
{
  Cel *src_cel, *dst_cel;

  ASSERT(src_layer != NULL);
  ASSERT(dst_layer != NULL);
  ASSERT(src_frame >= 0 && src_frame < sprite->getTotalFrames());
  ASSERT(dst_frame >= 0 && dst_frame < sprite->getTotalFrames());

  src_cel = static_cast<LayerImage*>(src_layer)->get_cel(src_frame);
  dst_cel = static_cast<LayerImage*>(dst_layer)->get_cel(dst_frame);

  if (undo_is_enabled(sprite->getUndo())) {
    undo_set_label(sprite->getUndo(), "Move Cel");
    undo_open(sprite->getUndo());

    undo_set_layer(sprite->getUndo(), sprite);
    undo_set_frame(sprite->getUndo(), sprite);
  }

  sprite->setCurrentLayer(dst_layer);
  sprite->setCurrentFrame(dst_frame);

  /* remove the 'dst_cel' (if it exists) because it must be
     replaced with 'src_cel' */
  if ((dst_cel != NULL) && (!dst_layer->is_background() || src_cel != NULL))
    remove_cel(sprite, static_cast<LayerImage*>(dst_layer), dst_cel);

  /* move the cel in the same layer */
  if (src_cel != NULL) {
    Image *src_image = stock_get_image(sprite->getStock(), src_cel->image);
    Image *dst_image;
    int image_index;
    int dst_cel_x;
    int dst_cel_y;
    int dst_cel_opacity;

    /* if we are moving a cel from a transparent layer to the
       background layer, we have to clear the background of the
       image */
    if (!src_layer->is_background() &&
	dst_layer->is_background()) {
      dst_image = image_crop(src_image,
			     -src_cel->x,
			     -src_cel->y,
			     sprite->getWidth(),
			     sprite->getHeight(), 0);

      image_clear(dst_image, app_get_color_to_clear_layer(dst_layer));
      image_merge(dst_image, src_image, src_cel->x, src_cel->y, 255, BLEND_MODE_NORMAL);

      dst_cel_x = 0;
      dst_cel_y = 0;
      dst_cel_opacity = 255;
    }
    else {
      dst_image = image_new_copy(src_image);
      dst_cel_x = src_cel->x;
      dst_cel_y = src_cel->y;
      dst_cel_opacity = src_cel->opacity;
    }

    /* add the image in the stock */
    image_index = stock_add_image(sprite->getStock(), dst_image);
    if (undo_is_enabled(sprite->getUndo()))
      undo_add_image(sprite->getUndo(), sprite->getStock(), image_index);

    /* create the new cel */
    dst_cel = cel_new(dst_frame, image_index);
    cel_set_position(dst_cel, dst_cel_x, dst_cel_y);
    cel_set_opacity(dst_cel, dst_cel_opacity);

    if (undo_is_enabled(sprite->getUndo()))
      undo_add_cel(sprite->getUndo(), dst_layer, dst_cel);

    static_cast<LayerImage*>(dst_layer)->add_cel(dst_cel);
  }

  if (undo_is_enabled(sprite->getUndo()))
    undo_close(sprite->getUndo());

  set_frame_to_handle(NULL, 0, NULL, 0);
}

static void remove_cel(Sprite* sprite, LayerImage *layer, Cel *cel)
{
  Image *image;
  Cel *it;
  int frame;
  bool used;

  if (sprite != NULL && layer->is_image() && cel != NULL) {
    /* find if the image that use the cel to remove, is used by
       another cels */
    used = false;
    for (frame=0; frame<sprite->getTotalFrames(); ++frame) {
      it = layer->get_cel(frame);
      if (it != NULL && it != cel && it->image == cel->image) {
	used = true;
	break;
      }
    }

    if (undo_is_enabled(sprite->getUndo()))
      undo_open(sprite->getUndo());

    if (!used) {
      /* if the image is only used by this cel, we can remove the
	 image from the stock */
      image = stock_get_image(sprite->getStock(), cel->image);

      if (undo_is_enabled(sprite->getUndo()))
	undo_remove_image(sprite->getUndo(), sprite->getStock(), cel->image);

      stock_remove_image(sprite->getStock(), image);
      image_free(image);
    }

    if (undo_is_enabled(sprite->getUndo())) {
      undo_remove_cel(sprite->getUndo(), layer, cel);
      undo_close(sprite->getUndo());
    }

    /* remove the cel */
    layer->remove_cel(cel);
    cel_free(cel);
  }
}
