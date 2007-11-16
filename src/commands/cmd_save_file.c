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

#include "jinete/base.h"

#include "commands/commands.h"
#include "console/console.h"
#include "file/file.h"
#include "modules/recent.h"
#include "modules/sprites.h"
#include "raster/sprite.h"

#endif

bool command_enabled_save_file(const char *argument)
{
  return current_sprite != NULL;
}

void command_execute_save_file(const char *argument)
{
  /* if the sprite is associated to a file in the file-system, we can
     save it directly without user interaction */
  if (sprite_is_associated_to_file(current_sprite)) {
    if (sprite_save(current_sprite) == 0) {
      recent_file(current_sprite->filename);
      sprite_mark_as_saved(current_sprite);
    }
    else {
      unrecent_file(current_sprite->filename);
      console_printf("%s: %s",
		     _("Error saving sprite file"),
		     current_sprite->filename);
    }
  }
  /* if the sprite isn't associated to a file, we must to show the
     save-as dialog to the user to select for first time the file-name
     for this sprite */
  else {
    command_execute(command_get_by_name(CMD_SAVE_FILE_AS), argument);
  }
}
