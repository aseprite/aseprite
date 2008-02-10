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

#ifndef USE_PRECOMPILED_HEADER

#include "jinete/jinete.h"

#include "commands/commands.h"
#include "console/console.h"
#include "core/app.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "raster/undo.h"

#endif

static Layer *duplicate_layer(void);

static bool cmd_duplicate_layer_enabled(const char *argument)
{
  return
    current_sprite != NULL &&
    current_sprite->layer != NULL;
}

static void cmd_duplicate_layer_execute(const char *argument)
{
  if (duplicate_layer() != NULL)
    update_screen_for_sprite(current_sprite);
}

static Layer *duplicate_layer(void)
{
  Sprite *sprite = current_sprite;
  Layer *layer_copy;
  char buf[1024];

  if (!sprite || !sprite->layer)
    return NULL;

  layer_copy = layer_new_copy(sprite->layer);
  if (!layer_copy) {
    console_printf("Not enough memory");
    return NULL;
  }

  sprintf(buf, "%s %s", layer_copy->name, _("Copy"));
  layer_set_name(layer_copy, buf);

  if (undo_is_enabled(sprite->undo)) {
    undo_open(sprite->undo);
    undo_add_layer(sprite->undo, (Layer *)sprite->layer->parent, layer_copy);
  }

  layer_add_layer((Layer *)sprite->layer->parent, layer_copy);

  if (undo_is_enabled(sprite->undo)) {
    undo_move_layer(sprite->undo, layer_copy);
    undo_set_layer(sprite->undo, sprite);
    undo_close(sprite->undo);
  }

  layer_move_layer((Layer *)sprite->layer->parent, layer_copy, sprite->layer);
  sprite_set_layer(sprite, layer_copy);

  return layer_copy;
}

Command cmd_duplicate_layer = {
  CMD_DUPLICATE_LAYER,
  cmd_duplicate_layer_enabled,
  NULL,
  cmd_duplicate_layer_execute,
  NULL
};
