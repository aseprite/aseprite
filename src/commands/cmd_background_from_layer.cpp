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
#include "raster/layer.h"
#include "raster/sprite.h"
#include "undoable.h"
#include "widgets/colbar.h"

static bool cmd_background_from_layer_enabled(const char *argument)
{
  const CurrentSpriteReader sprite;
  return
    sprite != NULL &&
    sprite->layer != NULL &&
    sprite_get_background_layer(sprite) == NULL &&
    layer_is_image(sprite->layer) &&
    layer_is_readable(sprite->layer) &&
    layer_is_writable(sprite->layer);
}

static void cmd_background_from_layer_execute(const char *argument)
{
  CurrentSpriteWriter sprite;

  // each frame of the layer to be converted as `Background' must be
  // cleared using the selected background color in the color-bar
  int bgcolor = app_get_bg_color(sprite);
  bgcolor = fixup_color_for_background(sprite->imgtype, bgcolor);

  {
    Undoable undoable(sprite, "Background from Layer");
    undoable.background_from_layer(sprite->layer, bgcolor);
    undoable.commit();
  }
  update_screen_for_sprite(sprite);
}

Command cmd_background_from_layer = {
  CMD_BACKGROUND_FROM_LAYER,
  cmd_background_from_layer_enabled,
  NULL,
  cmd_background_from_layer_execute,
};
