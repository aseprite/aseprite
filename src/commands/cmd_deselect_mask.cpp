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

static bool cmd_deselect_mask_enabled(const char *argument)
{
  return
    current_sprite != NULL &&
    !mask_is_empty(current_sprite->mask);
}

static void cmd_deselect_mask_execute(const char *argument)
{
  Sprite *sprite = current_sprite;
  Mask *mask;

  /* destroy the *deselected* mask */
  mask = sprite_request_mask(sprite, "*deselected*");
  if (mask) {
    sprite_remove_mask(sprite, mask);
    mask_free(mask);
  }

  /* save the selection in the repository */
  mask = mask_new_copy(sprite->mask);
  mask_set_name(mask, "*deselected*");
  sprite_add_mask(sprite, mask);

  /* undo */
  if (undo_is_enabled(sprite->undo)) {
    undo_set_label(sprite->undo, "Mask Deselection");
    undo_set_mask(sprite->undo, sprite);
  }

  /* deselect the mask */
  mask_none(sprite->mask);

  sprite_generate_mask_boundaries(sprite);
  update_screen_for_sprite(sprite);
}

Command cmd_deselect_mask = {
  CMD_DESELECT_MASK,
  cmd_deselect_mask_enabled,
  NULL,
  cmd_deselect_mask_execute,
  NULL
};
