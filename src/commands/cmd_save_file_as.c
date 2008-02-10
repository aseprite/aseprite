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

#ifndef USE_PRECOMPILED_HEADER

#include <allegro.h>

#include "jinete/jalert.h"

#include "commands/commands.h"
#include "console/console.h"
#include "core/app.h"
#include "dialogs/filesel.h"
#include "file/file.h"
#include "modules/recent.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/sprite.h"

#endif

static bool cmd_save_file_as_enabled(const char *argument)
{
  return current_sprite != NULL;
}

static void cmd_save_file_as_execute(const char *argument)
{
  char filename[4096];
  char exts[4096];
  char *newfilename;
  int ret;

  ustrcpy(filename, current_sprite->filename);
  get_writable_extensions(exts, sizeof(exts));

  for (;;) {
    newfilename = ase_file_selector(_("Save Sprite"), filename, exts);
    if (!newfilename)
      return;
    ustrcpy(filename, newfilename);
    jfree(newfilename);

    /* does the file exist? */
    if (exists(filename)) {
      /* ask if the user wants overwrite the file? */
      ret = jalert("%s<<%s<<%s||%s||%s||%s",
		   _("Warning"),
		   _("File exists, overwrite it?"),
		   get_filename(filename),
		   _("&Yes"), _("&No"), _("&Cancel"));
    }
    else
      break;

    /* "yes": we must continue with the operation... */
    if (ret == 1)
      break;
    /* "cancel" or <esc> per example: we back doing nothing */
    else if (ret != 2)
      return;

    /* "no": we must back to select other file-name */
  }

  sprite_set_filename(current_sprite, filename);
  app_realloc_sprite_list();

  if (sprite_save(current_sprite) == 0) {
    recent_file(filename);
    sprite_mark_as_saved(current_sprite);
  }
  else {
    /* TODO if the user cancel we shouldn't unrecent the file */
    unrecent_file(filename);
  }
}

Command cmd_save_file_as = {
  CMD_SAVE_FILE_AS,
  cmd_save_file_as_enabled,
  NULL,
  cmd_save_file_as_execute,
  NULL
};
