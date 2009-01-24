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

#include "jinete/jinete.h"

#include "commands/commands.h"
#include "modules/sprites.h"
#include "dialogs/aniedit.h"

static bool cmd_film_editor_enabled(const char *argument)
{
  return current_sprite != NULL;
}

static void cmd_film_editor_execute(const char *argument)
{
  switch_between_animation_and_sprite_editor();
}

Command cmd_film_editor = {
  CMD_FILM_EDITOR,
  cmd_film_editor_enabled,
  NULL,
  cmd_film_editor_execute,
  NULL
};
