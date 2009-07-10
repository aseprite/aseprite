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
#include "raster/sprite.h"
#include "undoable.h"
#include "widgets/colbar.h"

static bool cmd_flatten_layers_enabled(const char *argument)
{
  const CurrentSpriteReader sprite;
  return sprite != NULL;
}

static void cmd_flatten_layers_execute(const char *argument)
{
  CurrentSpriteWriter sprite;
  int bgcolor = get_color_for_image(sprite->imgtype,
				    colorbar_get_bg_color(app_get_colorbar()));
  {
    Undoable undoable(sprite, "Flatten Layers");
    undoable.flatten_layers(bgcolor);
    undoable.commit();
  }
  update_screen_for_sprite(sprite);
}

Command cmd_flatten_layers = {
  CMD_FLATTEN_LAYERS,
  cmd_flatten_layers_enabled,
  NULL,
  cmd_flatten_layers_execute,
};
