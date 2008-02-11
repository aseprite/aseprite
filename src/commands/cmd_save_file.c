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

#include <allegro.h>

#include "jinete/jbase.h"

#include "commands/commands.h"
#include "core/app.h"
#include "console/console.h"
#include "file/file.h"
#include "modules/recent.h"
#include "modules/sprites.h"
#include "raster/sprite.h"
#include "widgets/statebar.h"

static bool cmd_save_file_enabled(const char *argument)
{
  return current_sprite != NULL;
}

static void cmd_save_file_execute(const char *argument)
{
  /* if the sprite is associated to a file in the file-system, we can
     save it directly without user interaction */
  if (sprite_is_associated_to_file(current_sprite)) {
    if (sprite_save(current_sprite) == 0) {
      recent_file(current_sprite->filename);
      sprite_mark_as_saved(current_sprite);

      if (app_get_status_bar())
	status_bar_set_text(app_get_status_bar(),
			    1000, "File %s, saved.",
			    get_filename(current_sprite->filename));
    }
    else {
      /* TODO if the user cancel we shouldn't unrecent the file */
      unrecent_file(current_sprite->filename);
    }
  }
  /* if the sprite isn't associated to a file, we must to show the
     save-as dialog to the user to select for first time the file-name
     for this sprite */
  else {
    command_execute(command_get_by_name(CMD_SAVE_FILE_AS), argument);
  }
}

Command cmd_save_file = {
  CMD_SAVE_FILE,
  cmd_save_file_enabled,
  NULL,
  cmd_save_file_execute,
  NULL
};
