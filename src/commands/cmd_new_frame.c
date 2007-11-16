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
#include "raster/image.h"
#include "raster/frame.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "raster/undo.h"
#include "widgets/colbar.h"

#endif

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
  int bg, image_index;
  Frame *frame;
  Image *image;
  int frpos;

  frpos = current_sprite->frpos;
  while (layer_get_frame(current_sprite->layer, frpos))
    frpos++;

  /* create a new empty frame with a new clean image */
  image = image_new(current_sprite->imgtype,
		    current_sprite->w,
		    current_sprite->h);
  if (!image) {
    console_printf(_("Not enough memory.\n"));
    return;
  }

  /* background color (right color) */
  bg = get_color_for_image(image->imgtype,
			   color_bar_get_color(app_get_color_bar(), 1));
  image_clear(image, bg);

  /* add the image in the stock */
  image_index = stock_add_image(current_sprite->layer->stock, image);

  undo_open(current_sprite->undo);

  if (frpos >= current_sprite->frames) {
    undo_set_frames(current_sprite->undo, current_sprite);
    sprite_set_frames(current_sprite, frpos+1);
  }
  undo_int(current_sprite->undo, &current_sprite->gfxobj, &current_sprite->frpos);
  sprite_set_frpos(current_sprite, frpos);

  undo_add_image(current_sprite->undo,
		 current_sprite->layer->stock, image);

  /* add the frame in the layer */
  frame = frame_new(current_sprite->frpos, image_index);
  undo_add_frame(current_sprite->undo, current_sprite->layer, frame);
  layer_add_frame(current_sprite->layer, frame);

  undo_close(current_sprite->undo);

  GUI_Refresh(current_sprite);
}
