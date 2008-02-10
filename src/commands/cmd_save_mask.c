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

#include <allegro/file.h>

#include "jinete/jalert.h"

#include "commands/commands.h"
#include "dialogs/filesel.h"
#include "modules/sprites.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "util/msk_file.h"

#endif

static bool cmd_save_mask_enabled(const char *argument)
{
  if (!current_sprite)
    return FALSE;
  else
    return (current_sprite->mask &&
	    current_sprite->mask->bitmap) ? TRUE: FALSE;
}

static void cmd_save_mask_execute(const char *argument)
{
  /* get current sprite */
  Sprite *sprite = current_sprite;
  char *filename = "default.msk";
  int ret;

  for (;;) {
    char *filename = ase_file_selector(_("Save .msk File"), filename, "msk");
    if (!filename)
      return;

    /* does the file exist? */
    if (exists(filename)) {
      /* ask if the user wants overwrite the file? */
      ret = jalert("%s<<%s<<%s||%s||%s||%s",
		   _("Warning"), _("File exists, overwrite it?"),
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

  if (save_msk_file(sprite->mask, filename) != 0)
    jalert("%s<<%s<<%s||%s",
	   _("Error"), _("Error saving .msk file"),
	   _(filename), _("&Close"));
}

Command cmd_save_mask = {
  CMD_SAVE_MASK,
  cmd_save_mask_enabled,
  NULL,
  cmd_save_mask_execute,
  NULL
};
