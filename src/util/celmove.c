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
  Sprite *sprite = current_sprite;

  if (handle_layer && handle_cel &&
      !layer_get_cel(sprite->layer,
		     sprite->frame)
/*       layer_get_cel(sprite->layer, */
/* 		    sprite->frame) != handle_cel */) {
    undo_open(sprite->undo);

    /* is a cel in the destination layer/frame? */
/*     if (layer_get_cel(sprite->layer, sprite->frame)) { */
/*       bool increment_frames = FALSE; */
/*       Cel *cel; */
/*       int c; */

/*       for (c=sprite->frames-1; c>=sprite->frame; --c) { */
/* 	cel = layer_get_cel(sprite->layer, c); */
/* 	if (cel) { */
/* 	  undo_int(sprite->undo, &cel->gfxobj, &cel->frame); */
/* 	  cel->frame++; */
/* 	  if (cel->frame == sprite->frames) */
/* 	    increment_frames = TRUE; */
/* 	} */
/*       } */

/*       /\* increment frames counter in the sprite *\/ */
/*       if (increment_frames) { */
/* 	undo_set_frames(sprite->undo, sprite); */
/* 	sprite_set_frames(sprite, sprite->frames+1); */
/*       } */
/*     } */

    /* move a cel in the same layer */
    if (handle_layer == sprite->layer) {
      undo_remove_cel(sprite->undo, handle_layer, handle_cel);
      handle_cel->frame = sprite->frame;
      undo_add_cel(sprite->undo, handle_layer, handle_cel);
    }
    /* move a cel from "handle_layer" to "sprite->layer" */
    else {
      Layer *handle_layer_bkp = handle_layer;
      Cel *handle_cel_bkp = handle_cel;

      copy_cel();
      RemoveCel(handle_layer_bkp, handle_cel_bkp);
    }

    undo_close(sprite->undo);
  }

  handle_layer = NULL;
  handle_cel = NULL;
}

void copy_cel(void)
{
  Sprite *sprite = current_sprite;

  if (handle_layer && handle_cel &&
      !layer_get_cel(sprite->layer,
		     sprite->frame)) {
    Cel *cel;
    Image *image;
    int image_index = 0;

    /* create a new cel with a new image (a copy of the
       "handle_cel" one) from "handle_layer" to "sprite->layer" */

    if (undo_is_enabled(sprite->undo))
      undo_open(sprite->undo);

    cel = cel_new_copy(handle_cel);

    if (stock_get_image(sprite->stock, handle_cel->image)) {
      /* create a copy of the image */
      image = image_new_copy(stock_get_image(sprite->stock,
					     handle_cel->image));

      /* add the image in the stock of current layer */
      image_index = stock_add_image(sprite->stock, image);

      if (undo_is_enabled(sprite->undo))
	undo_add_image(sprite->undo, sprite->stock, image);
    }

    /* setup the cel */
    cel_set_frame(cel, sprite->frame);
    cel_set_image(cel, image_index);

    /* add the cel in the current layer */
    if (undo_is_enabled(sprite->undo))
      undo_add_cel(sprite->undo, sprite->layer, cel);
    layer_add_cel(sprite->layer, cel);

    undo_close(sprite->undo);
  }

  handle_layer = NULL;
  handle_cel = NULL;
}

void link_cel(void)
{
  Sprite *sprite = current_sprite;

  if (handle_layer && handle_cel &&
      !layer_get_cel(sprite->layer,
		     sprite->frame)) {
    if (handle_layer == sprite->layer) {
      Cel *new_cel;

      /* create a new copy of the cel with the same image index but in
	 the new frame */
      new_cel = cel_new_copy(handle_cel);
      cel_set_frame(new_cel, sprite->frame);

      undo_add_cel(sprite->undo, handle_layer, new_cel);
      layer_add_cel(handle_layer, new_cel);
    }
    else {
      console_printf(_("Error: You can't link a cel from other layer\n"));
    }
  }

  handle_layer = NULL;
  handle_cel = NULL;
}
