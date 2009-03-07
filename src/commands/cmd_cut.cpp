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

#include "jinete/jbase.h"

#include "commands/commands.h"
#include "modules/sprites.h"
#include "raster/layer.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "util/clipboard.h"
#include "util/misc.h"

static bool cmd_cut_enabled(const char *argument)
{
  if ((!current_sprite) ||
      (!current_sprite->layer) ||
      (!layer_is_readable(current_sprite->layer)) ||
      (!layer_is_writable(current_sprite->layer)) ||
      (!current_sprite->mask) ||
      (!current_sprite->mask->bitmap))
    return FALSE;
  else
    return GetImage(current_sprite) ? TRUE: FALSE;
}

static void cmd_cut_execute(const char *argument)
{
  clipboard::cut(current_sprite);
}

Command cmd_cut = {
  CMD_CUT,
  cmd_cut_enabled,
  NULL,
  cmd_cut_execute,
  NULL
};
