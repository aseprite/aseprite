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

#include <allegro/unicode.h>

#include "commands/commands.h"
#include "core/app.h"
#include "modules/editors.h"
#include "modules/tools.h"
#include "widgets/statebar.h"

static void cmd_show_grid_execute(const char *argument)
{
  set_view_grid(get_view_grid() ? FALSE: TRUE);
  refresh_all_editors();
}

static void cmd_snap_to_grid_execute(const char *argument)
{
  char buf[512];

  set_use_grid(get_use_grid() ? FALSE: TRUE);
  refresh_all_editors();

  usprintf(buf, _("Snap to grid: %s"),
	   get_use_grid() ? _("On"):
			    _("Off"));

  statusbar_set_text(app_get_statusbar(), 250, buf);
}

Command cmd_show_grid = {
  CMD_SHOW_GRID,
  NULL,
  NULL,
  cmd_show_grid_execute,
  NULL
};

Command cmd_snap_to_grid = {
  CMD_SNAP_TO_GRID,
  NULL,
  NULL,
  cmd_snap_to_grid_execute,
  NULL
};
