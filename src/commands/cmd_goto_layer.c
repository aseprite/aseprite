/* ASE - Allegro Sprite Editor
 * Copyright (C) 2008  David A. Capello
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
#include "modules/editors.h"
#include "modules/sprites.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "widgets/editor.h"
#include "widgets/statebar.h"

/* ======================== */
/* goto_previous_layer      */
/* ======================== */

static bool cmd_goto_previous_layer_enabled(const char *argument)
{
  return current_sprite != NULL;
}

static void cmd_goto_previous_layer_execute(const char *argument)
{
  int i = sprite_layer2index(current_sprite, current_sprite->layer);
  
  if (i > 0)
    i--;
  else
    i = sprite_count_layers(current_sprite)-1;

  current_sprite->layer = sprite_index2layer(current_sprite, i);

  update_screen_for_sprite(current_sprite);
  editor_update_statusbar_for_standby(current_editor);

  statusbar_show_tip(app_get_statusbar(), 1000,
		     _("Layer `%s' selected"),
		     current_sprite->layer->name);
}

/* ======================== */
/* goto_next_layer          */
/* ======================== */

static bool cmd_goto_next_layer_enabled(const char *argument)
{
  return current_sprite != NULL;
}

static void cmd_goto_next_layer_execute(const char *argument)
{
  int i = sprite_layer2index(current_sprite, current_sprite->layer);

  if (i < sprite_count_layers(current_sprite)-1)
    i++;
  else
    i = 0;

  current_sprite->layer = sprite_index2layer(current_sprite, i);

  update_screen_for_sprite(current_sprite);
  editor_update_statusbar_for_standby(current_editor);

  statusbar_show_tip(app_get_statusbar(), 1000,
		     _("Layer `%s' selected"),
		     current_sprite->layer->name);
}

Command cmd_goto_previous_layer = {
  CMD_GOTO_PREVIOUS_LAYER,
  cmd_goto_previous_layer_enabled,
  NULL,
  cmd_goto_previous_layer_execute,
};

Command cmd_goto_next_layer = {
  CMD_GOTO_NEXT_LAYER,
  cmd_goto_next_layer_enabled,
  NULL,
  cmd_goto_next_layer_execute,
};
