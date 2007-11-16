/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2005, 2007  David A. Capello
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

#include "jinete/list.h"

#include "console/console.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/frame.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "raster/undo.h"
#include "util/misc.h"

#endif

static void displace_layers(Undo *undo, Layer *layer, int x, int y);

void crop_sprite(void)
{
  Sprite *sprite = current_sprite;

  if ((sprite) && (!mask_is_empty (sprite->mask))) {
    if (undo_is_enabled (sprite->undo)) {
      undo_open (sprite->undo);
      undo_int (sprite->undo, (GfxObj *)sprite, &sprite->w);
      undo_int (sprite->undo, (GfxObj *)sprite, &sprite->h);
    }

    sprite_set_size(sprite, sprite->mask->w, sprite->mask->h);

    displace_layers(sprite->undo, sprite->set,
		    -sprite->mask->x, -sprite->mask->y);

    if (undo_is_enabled(sprite->undo)) {
      undo_int(sprite->undo, (GfxObj *)sprite->mask, &sprite->mask->x);
      undo_int(sprite->undo, (GfxObj *)sprite->mask, &sprite->mask->y);
    }

    sprite->mask->x = 0;
    sprite->mask->y = 0;

    if (undo_is_enabled(sprite->undo))
      undo_close(sprite->undo);

    sprite_generate_mask_boundaries(sprite);
    GUI_Refresh(sprite);
  }
}

void crop_layer(void)
{
  Sprite *sprite = current_sprite;

  if ((sprite) && (!mask_is_empty(sprite->mask)) &&
      (sprite->layer) && (layer_is_image(sprite->layer))) {
    Layer *layer = sprite->layer;
    Frame *frame;
    Image *image;
    Layer *new_layer;
    Frame *new_frame;
    Image *new_image;
    Layer *set = (Layer *)layer->parent;
    JLink link;

    new_layer = layer_new(layer->imgtype);
    if (!new_layer) {
      console_printf(_("Not enough memory\n"));
      return;
    }

    layer_set_name (new_layer, layer->name);
    layer_set_blend_mode (new_layer, layer->blend_mode);

    JI_LIST_FOR_EACH(layer->frames, link) {
      frame = link->data;
      image = stock_get_image(layer->stock, frame->image);
      if (!image)
	continue;

      new_frame = frame_new_copy(frame);
      if (!new_frame) {
	layer_free(new_layer);
	console_printf(_("Not enough memory\n"));
	return;
      }

      new_image = image_crop(image,
			     sprite->mask->x-frame->x,
			     sprite->mask->y-frame->y,
			     sprite->mask->w,
			     sprite->mask->h);
      if (!new_image) {
	layer_free(new_layer);
	frame_free(new_frame);
	console_printf(_("Not enough memory\n"));
	return;
      }

      new_frame->image = stock_add_image(new_layer->stock, new_image);
      new_frame->x = sprite->mask->x;
      new_frame->y = sprite->mask->y;

      layer_add_frame(new_layer, new_frame);
    }

    /* add the new layer */
    if (undo_is_enabled(sprite->undo)) {
      undo_open(sprite->undo);
      undo_add_layer(sprite->undo, set, new_layer);
    }

    layer_add_layer(set, new_layer);

    /* move it after the old one */
    if (undo_is_enabled(sprite->undo))
      undo_move_layer(sprite->undo, new_layer);

    layer_move_layer(set, new_layer, layer);

    /* set the new one as the current one */
    if (undo_is_enabled(sprite->undo))
      undo_set_layer(sprite->undo, sprite);

    sprite_set_layer(sprite, new_layer);

    /* remove the old layer */
    if (undo_is_enabled(sprite->undo)) {
      undo_remove_layer(sprite->undo, layer);
      undo_close(sprite->undo);
    }

    layer_remove_layer(set, layer);
    layer_free(layer);

    /* refresh */
    GUI_Refresh(sprite);
  }
}

void crop_frame(void)
{
  Sprite *sprite = current_sprite;
  Image *image = GetImage ();

  if ((sprite) && (!mask_is_empty (sprite->mask)) && (image)) {
    Frame *frame = layer_get_frame (sprite->layer, sprite->frpos);

    /* undo */
    undo_open(sprite->undo);
    undo_int(sprite->undo, (GfxObj *)frame, &frame->x);
    undo_int(sprite->undo, (GfxObj *)frame, &frame->y);
    undo_replace_image(sprite->undo, sprite->layer->stock, frame->image);
    undo_close(sprite->undo);

    /* replace the image */
    sprite->layer->stock->image[frame->image] =
      image_crop(image,
		 sprite->mask->x-frame->x,
		 sprite->mask->y-frame->y,
		 sprite->mask->w,
		 sprite->mask->h);

    image_free(image);		/* destroy the old image */

    /* change the frame position */
    frame->x = sprite->mask->x;
    frame->y = sprite->mask->y;

    GUI_Refresh(sprite);
  }
}

/* Moves every frame in "layer" with the offset "x"/"y".  */

static void displace_layers(Undo *undo, Layer *layer, int x, int y)
{
  switch (layer->gfxobj.type) {

    case GFXOBJ_LAYER_IMAGE: {
      Frame *frame;
      JLink link;

      JI_LIST_FOR_EACH(layer->frames, link) {
	frame = link->data;

	if (undo_is_enabled(undo)) {
	  undo_int(undo, (GfxObj *)frame, &frame->x);
	  undo_int(undo, (GfxObj *)frame, &frame->y);
	}

	frame->x += x;
	frame->y += y;
      }
      break;
    }

    case GFXOBJ_LAYER_SET: {
      JLink link;
      JI_LIST_FOR_EACH(layer->layers, link)
	displace_layers(undo, link->data, x, y);
      break;
    }

    case GFXOBJ_LAYER_TEXT:
      /* XXX */
      break;
  }
}
