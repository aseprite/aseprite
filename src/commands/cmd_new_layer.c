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

#include "jinete/jinete.h"

#include "commands/commands.h"
#include "core/app.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "raster/undo.h"
#include "script/functions.h"

static bool cmd_new_layer_enabled(const char *argument)
{
  return current_sprite != NULL;
}

static void cmd_new_layer_execute(const char *argument)
{
  JWidget window, name_widget;
  Sprite *sprite = current_sprite; /* get current sprite */

  /* load the window widget */
  window = load_widget("newlay.jid", "new_layer");
  if (!window)
    return;

  name_widget = jwidget_find_name(window, "name");
  {
    char *name = GetUniqueLayerName(sprite);
    jwidget_set_text(name_widget, name);
    jfree(name);
  }
  jwidget_set_min_size(name_widget, 128, 0);

  jwindow_open_fg(window);

  if (jwindow_get_killer(window) == jwidget_find_name(window, "ok")) {
    const char *name = jwidget_get_text(jwidget_find_name(window, "name"));
    Layer *layer;

    if (undo_is_enabled(sprite->undo))
      undo_set_label(sprite->undo, "New Layer");

    layer = NewLayer(sprite);
    if (!layer) {
      jalert(_("Error<<Not enough memory||&Close"));
      return;
    }
    layer_set_name(layer, name);
    update_screen_for_sprite(sprite);
  }

  jwidget_free(window);
}

Command cmd_new_layer = {
  CMD_NEW_LAYER,
  cmd_new_layer_enabled,
  NULL,
  cmd_new_layer_execute,
  NULL
};
