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

#include "ui_context.h"
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
  const CurrentSpriteReader sprite;
  return sprite != NULL;
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
  return !UIContext::instance()->get_sprite_list().empty();
}

static void cmd_close_all_files_execute(const char *argument)
{
  UIContext* context = UIContext::instance();
  if (!context->get_current_sprite())
    context->show_sprite(context->get_first_sprite());

  while (true) {
    if (context->get_current_sprite() != NULL) {
      if (!close_current_sprite())
	break;
    }
    else
      break;
  }
}

/**
 * Closes the current sprite, asking to the user if to save it if it's
 * modified.
 */
static bool close_current_sprite()
{
  bool save_it;

try_again:;
  // This flag indicates if we have to sabe the sprite before to destroy it
  save_it = false;
  {
    // The sprite is locked as reader temporaly
    CurrentSpriteReader sprite;

    /* see if the sprite has changes */
    while (sprite_is_modified(sprite)) {
      /* ask what want to do the user with the changes in the sprite */
      int ret = jalert("%s<<%s<<%s||%s||%s||%s",
		       _("Warning"), _("Saving changes in:"),
		       get_filename(sprite->filename),
		       _("&Save"), _("&Discard"), _("&Cancel"));

      if (ret == 1) {
	/* "save": save the changes */
	save_it = true;
	break;
      }
      else if (ret != 2) {
	/* "cancel" or "ESC" */
	return false; /* we back doing nothing */
      }
      else {
	/* "discard" */
	break;
      }
    }
  }

  // Does we need to save the sprite?
  if (save_it) {
    // TODO we have to pass the sprite to the save file command
    command_execute(command_get_by_name(CMD_SAVE_FILE), NULL);
    goto try_again;
  }

  // Destroy the sprite (locking it as writer)
  {
    CurrentSpriteWriter sprite;
    sprite.destroy();
  }
  return true;
}

Command cmd_close_file = {
  CMD_CLOSE_FILE,
  cmd_close_file_enabled,
  NULL,
  cmd_close_file_execute,
};

Command cmd_close_all_files = {
  CMD_CLOSE_ALL_FILES,
  cmd_close_all_files_enabled,
  NULL,
  cmd_close_all_files_execute,
};
