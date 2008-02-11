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

#include "jinete/jinete.h"

#include "commands/commands.h"
#include "script/script.h"

static void cmd_run_script_execute(const char *argument)
{
  char *filename = ji_file_select(_("Load Script File(.lua)"), "", "lua");
  if (filename) {
    do_script_file(filename);
    jfree(filename);
  }
}

Command cmd_run_script = {
  CMD_RUN_SCRIPT,
  NULL,
  NULL,
  cmd_run_script_execute,
  NULL
};
