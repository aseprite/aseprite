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

#include "commands/commands.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "raster/undo.h"

static bool cmd_mask_all_enabled(const char *argument)
{
  return current_sprite != NULL;
}
  
static void cmd_mask_all_execute(const char *argument)
{
  Sprite *sprite = current_sprite;

  /* undo */
  if (undo_is_enabled(sprite->undo)) {
    undo_set_label(sprite->undo, "Mask All");
    undo_set_mask(sprite->undo, sprite);
  }

  /* change the selection */
  mask_replace(sprite->mask, 0, 0, sprite->w, sprite->h);

  sprite_generate_mask_boundaries(sprite);
  update_screen_for_sprite(sprite);
}

Command cmd_mask_all = {
  CMD_MASK_ALL,
  cmd_mask_all_enabled,
  NULL,
  cmd_mask_all_execute,
  NULL
};
