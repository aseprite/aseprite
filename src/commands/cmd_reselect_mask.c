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

bool command_enabled_reselect_mask(const char *argument)
{
  return
    current_sprite != NULL &&
    sprite_request_mask(current_sprite, "*deselected*") != NULL;
}

void command_execute_reselect_mask(const char *argument)
{
  Sprite *sprite = current_sprite;
  Mask *mask;

  /* request *deselected* mask */
  mask = sprite_request_mask(sprite, "*deselected*");

  /* undo */
  if (undo_is_enabled(sprite->undo))
    undo_set_mask(sprite->undo, sprite);

  /* set the mask */
  sprite_set_mask(sprite, mask);

  /* remove the *deselected* mask */
  sprite_remove_mask(sprite, mask);
  mask_free(mask);

  sprite_generate_mask_boundaries(sprite);
  update_screen_for_sprite(sprite);
}
