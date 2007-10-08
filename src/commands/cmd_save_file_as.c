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

#include <allegro.h>

#include "jinete/alert.h"

#include "console/console.h"
/* #include "core/app.h" */
#include "dialogs/filesel.h"
#include "file/file.h"
#include "modules/recent.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/sprite.h"

#endif

bool command_enabled_save_file_as(const char *argument)
{
  return current_sprite != NULL;
}

void command_execute_save_file_as(const char *argument)
{
  char filename[4096];
  char *newfilename;
  int ret;

  ustrcpy(filename, current_sprite->filename);

  for (;;) {
    newfilename = GUI_FileSelect(_("Save Sprite"), filename,
				 get_writable_extensions());
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
  rebuild_sprite_list();

  if (sprite_save(current_sprite) == 0) {
    recent_file(filename);
    sprite_mark_as_saved(current_sprite);
  }
  else {
    unrecent_file(filename);
    console_printf("%s: %s",
		   _("Error saving sprite file"),
		   filename);
  }
}
