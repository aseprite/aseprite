/* ASE - Allegro Sprite Editor
 * Copyright (C) 2007  David A. Capello
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

#ifndef USE_PRECOMPILED_HEADER

#include "jinete.h"

#include "core/app.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/frame.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "raster/undo.h"

#endif

bool command_enabled_merge_down_layer(const char *argument)
{
  Layer *src_layer, *dst_layer;
  Sprite *sprite;

  sprite = current_sprite;
  if (!sprite)
    return FALSE;

  src_layer = sprite->layer;
  if (!src_layer || !layer_is_image(src_layer))
    return FALSE;

  dst_layer = layer_get_prev(sprite->layer);
  if (!dst_layer || !layer_is_image(dst_layer))
    return FALSE;

  return TRUE;
}

void command_execute_merge_down_layer(const char *argument)
{
  Sprite *sprite = current_sprite;
  Layer *src_layer, *dst_layer;
  Frame *src_frame, *dst_frame;
  Image *src_image, *dst_image;
  int frpos, index;

  src_layer = sprite->layer;
  dst_layer = layer_get_prev(sprite->layer);

  if (undo_is_enabled(sprite->undo))
    undo_open(sprite->undo);

  for (frpos=0; frpos<sprite->frames; ++frpos) {
    /* get frames */
    src_frame = layer_get_frame(src_layer, frpos);
    dst_frame = layer_get_frame(dst_layer, frpos);

    /* get images */
    if (src_frame)
      src_image = stock_get_image(src_layer->stock, src_frame->image);
    else
      src_image = NULL;

    if (dst_frame)
      dst_image = stock_get_image(dst_layer->stock, dst_frame->image);
    else
      dst_image = NULL;

    /* with source image? */
    if (src_image) {
      /* no destination image */
      if (!dst_image) {
	/* copy this frame to the destination layer */
	dst_image = image_new_copy(src_image);
	index = stock_add_image(dst_layer->stock, dst_image);
	if (undo_is_enabled(sprite->undo)) {
	  undo_add_image(sprite->undo, dst_layer->stock, dst_image);
	}
	dst_frame = frame_new(frpos, index);
	frame_set_position(dst_frame, src_frame->x, src_frame->y);
	frame_set_opacity(dst_frame, src_frame->opacity);
	if (undo_is_enabled(sprite->undo)) {
	  undo_add_frame(sprite->undo, dst_layer, dst_frame);
	}
	layer_add_frame(dst_layer, dst_frame);
      }
      /* with destination */
      else {
	int x1 = MIN(src_frame->x, dst_frame->x);
	int y1 = MIN(src_frame->y, dst_frame->y);
	int x2 = MAX(src_frame->x+src_image->w-1, dst_frame->x+dst_image->w-1);
	int y2 = MAX(src_frame->y+src_image->h-1, dst_frame->y+dst_image->h-1);
	Image *new_image = image_crop(dst_image,
				      x1-dst_frame->x,
				      y1-dst_frame->y,
				      x2-x1+1, y2-y1+1);

	/* merge src_image in new_image */
	image_merge(new_image, src_image,
		    src_frame->x-x1,
		    src_frame->y-y1,
		    src_frame->opacity,
		    src_layer->blend_mode);

	frame_set_position(dst_frame, x1, y1);
	if (undo_is_enabled(sprite->undo)) {
	  undo_replace_image(sprite->undo, dst_layer->stock, dst_frame->image);
	}
	stock_replace_image(dst_layer->stock, dst_frame->image, new_image);

	image_free(dst_image);
      }
    }
  }

  if (undo_is_enabled(sprite->undo)) {
    undo_set_layer(sprite->undo, sprite);
    undo_remove_layer(sprite->undo, src_layer);
    undo_close(sprite->undo);
  }

  sprite_set_layer(sprite, dst_layer);
  layer_remove_layer((Layer *)src_layer->parent, src_layer);
  layer_free(src_layer);

  GUI_Refresh(sprite);
}
