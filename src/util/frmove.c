/* ase -- allegro-sprite-editor: the ultimate sprites factory
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
#include "raster/frame.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "raster/undo.h"
#include "script/functions.h"
#include "util/frmove.h"
#include "widgets/colbar.h"

#endif

/* these variables indicate what frame to move (and the current_sprite
   frame indicates to where move it) */
static Layer *handle_layer = NULL;
static Frame *handle_frame = NULL;

void set_frame_to_handle (Layer *layer, Frame *frame)
{
  handle_layer = layer;
  handle_frame = frame;
}

void move_frame (void)
{
  if (handle_layer && handle_frame &&
      !layer_get_frame (current_sprite->layer,
			current_sprite->frpos)) {
    /* move a frame in the same layer */
    if (handle_layer == current_sprite->layer) {
      undo_open (current_sprite->undo);
      undo_remove_frame (current_sprite->undo, handle_layer, handle_frame);

      handle_frame->frpos = current_sprite->frpos;

      undo_add_frame (current_sprite->undo, handle_layer, handle_frame);
      undo_close (current_sprite->undo);
    }
    /* move a frame from "handle_layer" to "current_sprite->layer" */
    else {
      Layer *handle_layer_bkp = handle_layer;
      Frame *handle_frame_bkp = handle_frame;

      undo_open (current_sprite->undo);
      copy_frame ();
      RemoveFrame (handle_layer_bkp, handle_frame_bkp);
      undo_close (current_sprite->undo);
    }
  }

  handle_layer = NULL;
  handle_frame = NULL;
}

void copy_frame (void)
{
  if (handle_layer && handle_frame &&
      !layer_get_frame (current_sprite->layer,
			current_sprite->frpos)) {
    Frame *frame;
    Image *image;
    int image_index = 0;

    /* create a new frame with a new image (a copy of the
       "handle_frame" one) from "handle_layer" to "current_sprite->layer" */

    undo_open (current_sprite->undo);

    frame = frame_new_copy (handle_frame);

    if (stock_get_image (handle_layer->stock, handle_frame->image)) {
      /* create a copy of the image */
      image = image_new_copy (stock_get_image (handle_layer->stock,
					       handle_frame->image));

      /* add the image in the stock of current layer */
      image_index = stock_add_image (current_sprite->layer->stock, image);
      undo_add_image (current_sprite->undo,
		      current_sprite->layer->stock, image);
    }

    /* setup the frame */
    frame_set_frpos (frame, current_sprite->frpos);
    frame_set_image (frame, image_index);

    /* add the frame in the current layer */
    undo_add_frame (current_sprite->undo, current_sprite->layer, frame);
    layer_add_frame (current_sprite->layer, frame);

    undo_close (current_sprite->undo);
  }

  handle_layer = NULL;
  handle_frame = NULL;
}

void link_frame (void)
{
  if (handle_layer && handle_frame &&
      !layer_get_frame (current_sprite->layer,
			current_sprite->frpos)) {
    if (handle_layer == current_sprite->layer) {
      Frame *new_frame;

      /* create a new copy of the frame with the same image index but in
	 the new frame position */
      new_frame = frame_new_copy (handle_frame);
      frame_set_frpos (new_frame, current_sprite->frpos);

      undo_add_frame (current_sprite->undo, handle_layer, new_frame);
      layer_add_frame (handle_layer, new_frame);
    }
    else {
      console_printf (_("Error: You can't link a frame from other layer\n"));
    }
  }

  handle_layer = NULL;
  handle_frame = NULL;
}
