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

#include "console/console.h"
#include "core/app.h"
#include "core/core.h"
#include "modules/color.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "raster/undo.h"
#include "script/functions.h"
#include "util/celmove.h"
#include "widgets/colbar.h"

#endif

/* these variables indicate what cel to move (and the current_sprite
   frame indicates to where move it) */
static Layer *handle_layer = NULL;
static Cel *handle_cel = NULL;

void set_cel_to_handle(Layer *layer, Cel *cel)
{
  handle_layer = layer;
  handle_cel = cel;
}

void move_cel(void)
{
  if (handle_layer && handle_cel &&
      !layer_get_cel(current_sprite->layer,
		       current_sprite->frpos)) {
    /* move a cel in the same layer */
    if (handle_layer == current_sprite->layer) {
      undo_open (current_sprite->undo);
      undo_remove_cel (current_sprite->undo, handle_layer, handle_cel);

      handle_cel->frpos = current_sprite->frpos;

      undo_add_cel (current_sprite->undo, handle_layer, handle_cel);
      undo_close (current_sprite->undo);
    }
    /* move a cel from "handle_layer" to "current_sprite->layer" */
    else {
      Layer *handle_layer_bkp = handle_layer;
      Cel *handle_cel_bkp = handle_cel;

      undo_open(current_sprite->undo);
      copy_cel();
      RemoveCel(handle_layer_bkp, handle_cel_bkp);
      undo_close(current_sprite->undo);
    }
  }

  handle_layer = NULL;
  handle_cel = NULL;
}

void copy_cel(void)
{
  if (handle_layer && handle_cel &&
      !layer_get_cel(current_sprite->layer,
		       current_sprite->frpos)) {
    Cel *cel;
    Image *image;
    int image_index = 0;

    /* create a new cel with a new image (a copy of the
       "handle_cel" one) from "handle_layer" to "current_sprite->layer" */

    undo_open(current_sprite->undo);

    cel = cel_new_copy(handle_cel);

    if (stock_get_image(handle_layer->stock, handle_cel->image)) {
      /* create a copy of the image */
      image = image_new_copy(stock_get_image(handle_layer->stock,
					     handle_cel->image));

      /* add the image in the stock of current layer */
      image_index = stock_add_image(current_sprite->layer->stock, image);
      undo_add_image(current_sprite->undo,
		     current_sprite->layer->stock, image);
    }

    /* setup the cel */
    cel_set_frpos(cel, current_sprite->frpos);
    cel_set_image(cel, image_index);

    /* add the cel in the current layer */
    undo_add_cel(current_sprite->undo, current_sprite->layer, cel);
    layer_add_cel(current_sprite->layer, cel);

    undo_close(current_sprite->undo);
  }

  handle_layer = NULL;
  handle_cel = NULL;
}

void link_cel(void)
{
  if (handle_layer && handle_cel &&
      !layer_get_cel (current_sprite->layer,
			current_sprite->frpos)) {
    if (handle_layer == current_sprite->layer) {
      Cel *new_cel;

      /* create a new copy of the cel with the same image index but in
	 the new frame */
      new_cel = cel_new_copy(handle_cel);
      cel_set_frpos(new_cel, current_sprite->frpos);

      undo_add_cel(current_sprite->undo, handle_layer, new_cel);
      layer_add_cel(handle_layer, new_cel);
    }
    else {
      console_printf(_("Error: You can't link a cel from other layer\n"));
    }
  }

  handle_layer = NULL;
  handle_cel = NULL;
}
