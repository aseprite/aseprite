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

#include <allegro/unicode.h>

#include "jinete/jlist.h"

#include "commands/commands.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "raster/undo.h"
#include "util/misc.h"
#include "undoable.h"

static bool cmd_flip_enabled(const char *argument)
{
  assert(argument);
  const CurrentSpriteReader sprite;
  return
    sprite != NULL;
}

static void cmd_flip_execute(const char *argument)
{
  CurrentSpriteWriter sprite;
  bool flip_mask       = ustrstr(argument, "mask") != NULL;
  bool flip_canvas     = ustrstr(argument, "canvas") != NULL;
  bool flip_horizontal = ustrstr(argument, "horizontal") != NULL;
  bool flip_vertical   = ustrstr(argument, "vertical") != NULL;

  {
    Undoable undoable(sprite,
		      flip_mask ? (flip_horizontal ? "Flip Horizontal":
						     "Flip Vertical"):
				  (flip_horizontal ? "Flip Canvas Horizontal":
						     "Flip Canvas Vertical"));

    if (flip_mask) {
      Image *image, *area;
      int x1, y1, x2, y2;
      int x, y;

      image = GetImage2(sprite, &x, &y, NULL);
      if (!image)
	return;

      // mask is empty?
      if (mask_is_empty(sprite->mask)) {
	// so we flip the entire image
	x1 = 0;
	y1 = 0;
	x2 = image->w-1;
	y2 = image->h-1;
      }
      else {
	// apply the cel offset
	x1 = sprite->mask->x - x;
	y1 = sprite->mask->y - y;
	x2 = sprite->mask->x + sprite->mask->w - 1 - x;
	y2 = sprite->mask->y + sprite->mask->h - 1 - y;

	// clip
	x1 = MID(0, x1, image->w-1);
	y1 = MID(0, y1, image->h-1);
	x2 = MID(0, x2, image->w-1);
	y2 = MID(0, y2, image->h-1);
      }

      undoable.flip_image(image, x1, y1, x2, y2, flip_horizontal, flip_vertical);
    }
    else if (flip_canvas) {
      // get all sprite cels
      JList cels = jlist_new();
      sprite_get_cels(sprite, cels);

      // for each cel...
      JLink link;
      JI_LIST_FOR_EACH(cels, link) {
	Cel* cel = (Cel*)link->data;
	Image* image = stock_get_image(sprite->stock, cel->image);

	undoable.set_cel_position(cel,
				  flip_horizontal ? sprite->w - image->w - cel->x: cel->x,
				  flip_vertical ? sprite->h - image->h - cel->y: cel->y);

	undoable.flip_image(image, 0, 0, image->w-1, image->h-1,
			    flip_horizontal, flip_vertical);
      }
      jlist_free(cels);
    }

    undoable.commit();
  }
  update_screen_for_sprite(sprite);
}

Command cmd_flip = {
  CMD_FLIP,
  cmd_flip_enabled,
  NULL,
  cmd_flip_execute,
};
