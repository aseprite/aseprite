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

#include <allegro.h>
#include "jinete/jinete.h"

#include "commands/commands.h"
#include "core/app.h"
#include "modules/sprites.h"
#include "raster/sprite.h"

static bool close_current_sprite();

/* ======================== */
/* close_file               */
/* ======================== */

static bool cmd_close_file_enabled(const char *argument)
{
  return current_sprite != NULL;
}

static void cmd_close_file_execute(const char *argument)
{
  close_current_sprite();
}

/* ======================== */
/* close_all_files          */
/* ======================== */

static bool cmd_close_all_files_enabled(const char *argument)
{
  return !jlist_empty(get_sprite_list());
}

static void cmd_close_all_files_execute(const char *argument)
{
  if (!current_sprite)
    sprite_show(get_first_sprite());

  while (current_sprite != NULL &&
	 close_current_sprite())
    ;
}

/**
 * Closes the current sprite, asking to the user if to save it if it's
 * modified.
 */
static bool close_current_sprite()
{
  Sprite *sprite = current_sprite;

  /* see if the sprite has changes */
  while (sprite_is_modified(sprite)) {
    /* ask what want to do the user with the changes in the sprite */
    int ret = jalert("%s<<%s<<%s||%s||%s||%s",
		     _("Warning"), _("Saving changes in:"),
		     get_filename(sprite->filename),
		     _("&Save"), _("&Discard"), _("&Cancel"));

    if (ret == 1) {
      /* "save": save the changes */
      command_execute(command_get_by_name(CMD_SAVE_FILE), NULL);
    }
    else if (ret != 2) {
      /* "cancel" or "ESC" */
      return FALSE; /* we back doing nothing */
    }
    else {
      /* "discard" */
      break;
    }
  }

  /* closes the sprite */
  sprite_unmount(sprite);
  sprite_free(sprite);
  return TRUE;
}

Command cmd_close_file = {
  CMD_CLOSE_FILE,
  cmd_close_file_enabled,
  NULL,
  cmd_close_file_execute,
  NULL
};

Command cmd_close_all_files = {
  CMD_CLOSE_ALL_FILES,
  cmd_close_all_files_enabled,
  NULL,
  cmd_close_all_files_execute,
  NULL
};
