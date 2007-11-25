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

#include "console/console.h"
#include "core/app.h"
#include "modules/gui.h"
#include "modules/color.h"
#include "modules/sprites.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "raster/undo.h"
#include "widgets/colbar.h"

#endif

static bool new_frame_for_layer(Sprite *sprite, Layer *layer, int frame);
static bool copy_cel_in_next_frame(Sprite *sprite, Layer *layer, int frame);

bool command_enabled_new_frame(const char *argument)
{
  return
    current_sprite &&
    current_sprite->layer &&
    current_sprite->layer->readable &&
    current_sprite->layer->writable &&
    layer_is_image(current_sprite->layer);
}

void command_execute_new_frame(const char *argument)
{
  Sprite *sprite = current_sprite;

  undo_open(sprite->undo);

  /* add a new cel to every layer */
  new_frame_for_layer(sprite,
		      sprite->set,
		      sprite->frame+1);

  /* increment frames counter in the sprite */
  undo_set_frames(sprite->undo, sprite);
  sprite_set_frames(sprite, sprite->frames+1);

  /* go to next frame (the new one) */
  undo_int(sprite->undo, &sprite->gfxobj, &sprite->frame);
  sprite_set_frame(sprite, sprite->frame+1);

  /* close undo & refresh the screen */
  undo_close(sprite->undo);
  update_screen_for_sprite(sprite);
}

static bool new_frame_for_layer(Sprite *sprite, Layer *layer, int frame)
{
  switch (layer->gfxobj.type) {

    case GFXOBJ_LAYER_IMAGE: {
      Cel *cel;
      int c;

      for (c=sprite->frames-1; c>=frame; --c) {
	cel = layer_get_cel(layer, c);
	if (cel) {
	  undo_int(sprite->undo, &cel->gfxobj, &cel->frame);
	  cel->frame++;
	}
      }

      if (!copy_cel_in_next_frame(sprite, layer, frame))
	return FALSE;

      break;
    }

    case GFXOBJ_LAYER_SET: {
      JLink link;
      JI_LIST_FOR_EACH(layer->layers, link) {
	if (!new_frame_for_layer(sprite, link->data, frame))
	  return FALSE;
      }
      break;
    }

    case GFXOBJ_LAYER_TEXT:
      /* TODO */
      break;
  }

  return TRUE;
}

static bool copy_cel_in_next_frame(Sprite *sprite, Layer *layer, int frame)
{
  int bg, image_index;
  Image *image;
  Cel *cel;

  /* create a new empty cel with a new clean image */
  image = image_new(sprite->imgtype, sprite->w, sprite->h);
  if (!image) {
    console_printf(_("Not enough memory.\n"));
    return FALSE;
  }

  /* background color (right color) */
  bg = get_color_for_image(image->imgtype,
			   color_bar_get_color(app_get_color_bar(), 1));
  image_clear(image, bg);

  /* add the image in the stock */
  image_index = stock_add_image(layer->stock, image);

  undo_add_image(sprite->undo, layer->stock, image);

  /* add the cel in the layer */
  cel = cel_new(frame, image_index);
  undo_add_cel(sprite->undo, layer, cel);
  layer_add_cel(layer, cel);

  return TRUE;
}
