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
#include "core/app.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/sprite.h"
#include "raster/undo.h"
#include "widgets/statebar.h"

static bool cmd_redo_enabled(const char *argument)
{
  const CurrentSpriteReader sprite;
  return
    sprite != NULL &&
    undo_can_redo(sprite->undo);
}

static void cmd_redo_execute(const char *argument)
{
  CurrentSpriteWriter sprite;

  statusbar_show_tip(app_get_statusbar(), 1000,
		     _("Redid %s"),
		     undo_get_next_redo_label(sprite->undo));

  undo_do_redo(sprite->undo);
  sprite_generate_mask_boundaries(sprite);
  update_screen_for_sprite(sprite);
}

Command cmd_redo = {
  CMD_REDO,
  cmd_redo_enabled,
  NULL,
  cmd_redo_execute,
  NULL
};
