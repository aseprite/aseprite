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

#include "commands/commands.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/cel.h"
#include "raster/layer.h"
#include "raster/undo.h"
#include "raster/sprite.h"
#include "util/functions.h"

static bool cmd_remove_cel_enabled(const char *argument)
{
  return
    current_sprite &&
    current_sprite->layer &&
    layer_is_readable(current_sprite->layer) &&
    layer_is_writable(current_sprite->layer) &&
    layer_is_image(current_sprite->layer) &&
    layer_get_cel(current_sprite->layer, current_sprite->frame);
}

static void cmd_remove_cel_execute(const char *argument)
{
  Sprite *sprite = current_sprite;
  Cel *cel = layer_get_cel(sprite->layer, sprite->frame);

  if (undo_is_enabled(sprite->undo))
    undo_set_label(sprite->undo, "Remove Cel");

  RemoveCel(sprite->layer, cel);
  update_screen_for_sprite(sprite);
}

Command cmd_remove_cel = {
  CMD_REMOVE_CEL,
  cmd_remove_cel_enabled,
  NULL,
  cmd_remove_cel_execute,
  NULL
};
