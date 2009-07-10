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
#include "core/app.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/layer.h"
#include "raster/sprite.h"

static bool cmd_new_layer_set_enabled(const char *argument)
{
  const CurrentSpriteReader sprite;
  return
    sprite != NULL;
}

static void cmd_new_layer_set_execute(const char *argument)
{
  CurrentSpriteWriter sprite;

  // load the window widget
  JWidgetPtr window(load_widget("newlay.jid", "new_layer_set"));

  jwindow_open_fg(window);

  if (jwindow_get_killer(window) == jwidget_find_name(window, "ok")) {
    const char *name = jwidget_get_text(jwidget_find_name(window, "name"));
    Layer *layer = layer_set_new(sprite);

    layer_set_name(layer, name);
    layer_add_layer(sprite->set, layer);
    sprite_set_layer(sprite, layer);

    update_screen_for_sprite(sprite);
  }
}

Command cmd_new_layer_set = {
  CMD_NEW_LAYER_SET,
  cmd_new_layer_set_enabled,
  NULL,
  cmd_new_layer_set_execute,
};
