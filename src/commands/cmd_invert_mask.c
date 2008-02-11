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

#include "commands/commands.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "raster/undo.h"

static bool cmd_invert_mask_enabled(const char *argument)
{
  return current_sprite != NULL;
}

static void cmd_invert_mask_execute(const char *argument)
{
  Sprite *sprite = current_sprite;
  Mask *mask;

  /* change the selection */
  if (!sprite->mask->bitmap) {
    command_execute(command_get_by_name(CMD_MASK_ALL), argument);
  }
  else {
    /* undo */
    if (undo_is_enabled(sprite->undo))
      undo_set_mask(sprite->undo, sprite);

    /* create a new mask */
    mask = mask_new();

    /* select all the sprite area */
    mask_replace(mask, 0, 0, sprite->w, sprite->h);

    /* remove in the new mask the current sprite marked region */
    image_rectfill(mask->bitmap,
		   sprite->mask->x, sprite->mask->y,
		   sprite->mask->x + sprite->mask->w-1,
		   sprite->mask->y + sprite->mask->h-1, 0);

    /* invert the current mask in the sprite */
    mask_invert(sprite->mask);
    if (sprite->mask->bitmap) {
      /* copy the inverted region in the new mask */
      image_copy(mask->bitmap, sprite->mask->bitmap,
		 sprite->mask->x, sprite->mask->y);
    }

    /* we need only need the area inside the sprite */
    mask_intersect(mask, 0, 0, sprite->w, sprite->h);

    /* set the new mask */
    sprite_set_mask(sprite, mask);
    mask_free(mask);

    sprite_generate_mask_boundaries(sprite);
    update_screen_for_sprite(sprite);
  }
}

Command cmd_invert_mask = {
  CMD_INVERT_MASK,
  cmd_invert_mask_enabled,
  NULL,
  cmd_invert_mask_execute,
  NULL
};
