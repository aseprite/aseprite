/* ase -- allegro-sprite-editor: the ultimate sprites factory
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

#include "jinete/alert.h"

#include "dialogs/filesel.h"
#include "modules/gui.h" 
#include "modules/sprites.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "raster/undo.h"
#include "util/msk_file.h"

#endif

bool command_enabled_load_mask(const char *argument)
{
  return current_sprite != NULL;
}

void command_execute_load_mask(const char *argument)
{
  /* get current sprite */
  Sprite *sprite = current_sprite;
  char *filename = GUI_FileSelect(_("Load .msk File"), "", "msk");
  if (filename) {
    Mask *mask = load_msk_file(filename);
    if (!mask) {
      jalert("%s<<%s<<%s||%s",
	     _("Error"), _("Error loading .msk file"),
	     filename, _("&Close"));

      jfree(filename);
      return;
    }

    /* undo */
    if (undo_is_enabled(sprite->undo))
      undo_set_mask(sprite->undo, sprite);

    sprite_set_mask(sprite, mask);
    mask_free(mask);

    sprite_generate_mask_boundaries(sprite);
    GUI_Refresh(sprite);
    jfree(filename);
  }
}
