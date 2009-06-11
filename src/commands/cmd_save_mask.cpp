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

#include <allegro/file.h>

#include "jinete/jalert.h"

#include "commands/commands.h"
#include "dialogs/filesel.h"
#include "modules/sprites.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "util/msk_file.h"

static bool cmd_save_mask_enabled(const char *argument)
{
  const CurrentSpriteReader sprite;
  if (!sprite)
    return false;
  else
    return (sprite->mask &&
	    sprite->mask->bitmap) ? true: false;
}

static void cmd_save_mask_execute(const char *argument)
{
  const CurrentSpriteReader sprite;
  jstring filename = "default.msk";
  int ret;

  for (;;) {
    filename = ase_file_selector(_("Save .msk File"), filename, "msk");
    if (filename.empty())
      return;

    /* does the file exist? */
    if (exists(filename.c_str())) {
      /* ask if the user wants overwrite the file? */
      ret = jalert("%s<<%s<<%s||%s||%s||%s",
		   _("Warning"), _("File exists, overwrite it?"),
		   get_filename(filename.c_str()),
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

  if (save_msk_file(sprite->mask, filename.c_str()) != 0)
    jalert("%s<<%s<<%s||%s",
	   _("Error"), _("Error saving .msk file"),
	   filename.c_str(), _("&Close"));
}

Command cmd_save_mask = {
  CMD_SAVE_MASK,
  cmd_save_mask_enabled,
  NULL,
  cmd_save_mask_execute,
  NULL
};
