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

#include "commands/commands.h"
#include "dialogs/filesel.h"
#include "file/file.h"
#include "raster/sprite.h"
#include "modules/recent.h"
#include "modules/sprites.h"

#endif

static void cmd_open_file_execute(const char *argument)
{
  char *filename;

  /* interactive */
  if (!argument) {
    filename = GUI_FileSelect(_("Open Sprite"), "",
			      get_readable_extensions());
  }
  /* load the file specified in the argument */
  else {
    filename = (char *)argument;
  }

  if (filename) {
    Sprite *sprite = sprite_load(filename);
    if (sprite) {
      recent_file(filename);
      sprite_mount(sprite);
      sprite_show(sprite);
    }
    else {
      unrecent_file(filename);
    }

    if (filename != argument)
      jfree(filename);
  }
}

Command cmd_open_file = {
  CMD_OPEN_FILE,
  NULL,
  NULL,
  cmd_open_file_execute,
  NULL
};
