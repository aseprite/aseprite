/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2008  David A. Capello
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
#include "console/console.h"
#include "core/color.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "raster/undo.h"

static bool cmd_new_cel_enabled(const char *argument)
{
  return
    current_sprite &&
    current_sprite->layer &&
    layer_is_readable(current_sprite->layer) &&
    layer_is_writable(current_sprite->layer) &&
    layer_is_image(current_sprite->layer) &&
    !layer_get_cel(current_sprite->layer, current_sprite->frame);
}

static void cmd_new_cel_execute(const char *argument)
{
  int image_index;
  Image *image;
  Cel *cel;

  /* create a new empty cel with a new clean image */
  image = image_new(current_sprite->imgtype,
		    current_sprite->w,
		    current_sprite->h);
  if (!image) {
    console_printf(_("Not enough memory.\n"));
    return;
  }

  /* background color (right color) */
  image_clear(image, 0);

  /* add the image in the stock */
  image_index = stock_add_image(current_sprite->stock, image);

  if (undo_is_enabled(current_sprite->undo)) {
    undo_set_label(current_sprite->undo, "New Cel");
    undo_open(current_sprite->undo);
    undo_add_image(current_sprite->undo,
		   current_sprite->stock, image);
  }

  /* create the new cel */
  cel = cel_new(current_sprite->frame, image_index);

  /* add the cel in the layer */
  if (undo_is_enabled(current_sprite->undo))
    undo_add_cel(current_sprite->undo, current_sprite->layer, cel);

  layer_add_cel(current_sprite->layer, cel);

  if (undo_is_enabled(current_sprite->undo))
    undo_close(current_sprite->undo);

  update_screen_for_sprite(current_sprite);
}

Command cmd_new_cel = {
  CMD_NEW_CEL,
  cmd_new_cel_enabled,
  NULL,
  cmd_new_cel_execute,
  NULL
};
