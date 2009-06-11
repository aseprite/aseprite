/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2009  David Capello
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

#include "jinete/jinete.h"

#include "commands/commands.h"
#include "core/app.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "raster/undo.h"

static bool cmd_merge_down_layer_enabled(const char *argument)
{
  const CurrentSpriteReader sprite;
  if (!sprite)
    return false;

  Layer *src_layer = sprite->layer;
  if (!src_layer || !layer_is_image(src_layer))
    return false;

  Layer* dst_layer = layer_get_prev(sprite->layer);
  if (!dst_layer || !layer_is_image(dst_layer))
    return false;

  return TRUE;
}

static void cmd_merge_down_layer_execute(const char *argument)
{
  CurrentSpriteWriter sprite;
  Layer *src_layer, *dst_layer;
  Cel *src_cel, *dst_cel;
  Image *src_image, *dst_image;
  int frpos, index;

  src_layer = sprite->layer;
  dst_layer = layer_get_prev(sprite->layer);

  if (undo_is_enabled(sprite->undo)) {
    undo_set_label(sprite->undo, "Merge Down Layer");
    undo_open(sprite->undo);
  }

  for (frpos=0; frpos<sprite->frames; ++frpos) {
    /* get frames */
    src_cel = layer_get_cel(src_layer, frpos);
    dst_cel = layer_get_cel(dst_layer, frpos);

    /* get images */
    if (src_cel != NULL)
      src_image = stock_get_image(sprite->stock, src_cel->image);
    else
      src_image = NULL;

    if (dst_cel != NULL)
      dst_image = stock_get_image(sprite->stock, dst_cel->image);
    else
      dst_image = NULL;

    /* with source image? */
    if (src_image != NULL) {
      /* no destination image */
      if (dst_image == NULL) {	/* only a transparent layer can have a null cel */
	/* copy this cel to the destination layer... */

	/* creating a copy of the image */
	dst_image = image_new_copy(src_image);

	/* adding it in the stock of images */
	index = stock_add_image(sprite->stock, dst_image);
	if (undo_is_enabled(sprite->undo))
	  undo_add_image(sprite->undo, sprite->stock, index);

	/* creating a copy of the cell */
	dst_cel = cel_new(frpos, index);
	cel_set_position(dst_cel, src_cel->x, src_cel->y);
	cel_set_opacity(dst_cel, src_cel->opacity);

	if (undo_is_enabled(sprite->undo))
	  undo_add_cel(sprite->undo, dst_layer, dst_cel);
	layer_add_cel(dst_layer, dst_cel);
      }
      /* with destination */
      else {
	int x1, y1, x2, y2, bgcolor;
	Image *new_image;

	/* merge down in the background layer */
	if (layer_is_background(dst_layer)) {
	  x1 = 0;
	  y1 = 0;
	  x2 = sprite->w;
	  y2 = sprite->h;
	  bgcolor = app_get_color_to_clear_layer(dst_layer);
	}
	/* merge down in a transparent layer */
	else {
	  x1 = MIN(src_cel->x, dst_cel->x);
	  y1 = MIN(src_cel->y, dst_cel->y);
	  x2 = MAX(src_cel->x+src_image->w-1, dst_cel->x+dst_image->w-1);
	  y2 = MAX(src_cel->y+src_image->h-1, dst_cel->y+dst_image->h-1);
	  bgcolor = 0;
	}

	new_image = image_crop(dst_image,
			       x1-dst_cel->x,
			       y1-dst_cel->y,
			       x2-x1+1, y2-y1+1, bgcolor);

	/* merge src_image in new_image */
	image_merge(new_image, src_image,
		    src_cel->x-x1,
		    src_cel->y-y1,
		    src_cel->opacity,
		    src_layer->blend_mode);

	if (undo_is_enabled(sprite->undo)) {
	  undo_int(sprite->undo, (GfxObj *)dst_cel, &dst_cel->x);
	  undo_int(sprite->undo, (GfxObj *)dst_cel, &dst_cel->y);
	}

	cel_set_position(dst_cel, x1, y1);

	if (undo_is_enabled(sprite->undo))
	  undo_replace_image(sprite->undo, sprite->stock, dst_cel->image);
	stock_replace_image(sprite->stock, dst_cel->image, new_image);

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
  layer_remove_layer(src_layer->parent_layer, src_layer);

  layer_free_images(src_layer);
  layer_free(src_layer);

  update_screen_for_sprite(sprite);
}

Command cmd_merge_down_layer = {
  CMD_MERGE_DOWN_LAYER,
  cmd_merge_down_layer_enabled,
  NULL,
  cmd_merge_down_layer_execute,
  NULL
};
