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

#include "commands/commands.h"
#include "modules/editors.h"

static void cmd_close_editor_execute(const char *argument)
{
  close_editor(current_editor);
}

static void cmd_make_unique_editor_execute(const char *argument)
{
  make_unique_editor(current_editor);
}

static void cmd_split_editor_horizontally_execute(const char *argument)
{
  split_editor(current_editor, JI_HORIZONTAL);
}

static void cmd_split_editor_vertically_execute(const char *argument)
{
  split_editor(current_editor, JI_VERTICAL);
}

Command cmd_close_editor = {
  CMD_CLOSE_EDITOR,
  NULL,
  NULL,
  cmd_close_editor_execute,
};

Command cmd_make_unique_editor = {
  CMD_MAKE_UNIQUE_EDITOR,
  NULL,
  NULL,
  cmd_make_unique_editor_execute,
};

Command cmd_split_editor_horizontally = {
  CMD_SPLIT_EDITOR_HORIZONTALLY,
  NULL,
  NULL,
  cmd_split_editor_horizontally_execute,
};

Command cmd_split_editor_vertically = {
  CMD_SPLIT_EDITOR_VERTICALLY,
  NULL,
  NULL,
  cmd_split_editor_vertically_execute,
};
