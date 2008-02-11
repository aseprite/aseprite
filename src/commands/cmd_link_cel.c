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

#include "jinete/jbase.h"

#include "commands/commands.h"
#include "dialogs/filmedit.h"
#include "util/celmove.h"

static bool cmd_link_cel_enabled(const char *argument)
{
  return is_movingcel();
}

static void cmd_link_cel_execute(const char *argument)
{
  link_cel();
}

Command cmd_link_cel = {
  CMD_LINK_CEL,
  cmd_link_cel_enabled,
  NULL,
  cmd_link_cel_execute,
  NULL
};
