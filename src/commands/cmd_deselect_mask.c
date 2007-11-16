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

#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "raster/undo.h"

#endif

bool command_enabled_deselect_mask(const char *argument)
{
  return
    current_sprite != NULL &&
    !mask_is_empty(current_sprite->mask);
}

void command_execute_deselect_mask(const char *argument)
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
  if (undo_is_enabled(sprite->undo))
    undo_set_mask(sprite->undo, sprite);

  /* deselect the mask */
  mask_none(sprite->mask);

  sprite_generate_mask_boundaries(sprite);
  GUI_Refresh(sprite);
}
