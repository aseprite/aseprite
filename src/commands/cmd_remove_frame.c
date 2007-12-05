/* ASE - Allegro Sprite Editor
 * Copyright (C) 2007  David A. Capello
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
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/cel.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "raster/undo.h"
#include "script/functions.h"

#endif

static void remove_frame_for_layer(Sprite *sprite, Layer *layer, int frame);

static bool cmd_remove_frame_enabled(const char *argument)
{
  return
    current_sprite != NULL &&
    current_sprite->frames > 1;
}

static void cmd_remove_frame_execute(const char *argument)
{
  Sprite *sprite = current_sprite;

  undo_open(sprite->undo);

  /* remove cels from this frame (and displace one position backward
     all next frames) */
  remove_frame_for_layer(sprite, sprite->set, sprite->frame);

  /* decrement frames counter in the sprite */
  undo_set_frames(sprite->undo, sprite);
  sprite_set_frames(sprite, sprite->frames-1);

  /* move backward if we are outside the range of frames */
  if (sprite->frame >= sprite->frames) {
    undo_int(sprite->undo, &sprite->gfxobj, &sprite->frame);
    sprite_set_frame(sprite, sprite->frames-1);
  }

  undo_close(sprite->undo);
  update_screen_for_sprite(current_sprite);
}

static void remove_frame_for_layer(Sprite *sprite, Layer *layer, int frame)
{
  switch (layer->gfxobj.type) {

    case GFXOBJ_LAYER_IMAGE: {
      Cel *cel = layer_get_cel(layer, frame);
      if (cel)
	RemoveCel(layer, cel);

      for (++frame; frame<sprite->frames; ++frame) {
	cel = layer_get_cel(layer, frame);
	if (cel) {
	  undo_int(sprite->undo, &cel->gfxobj, &cel->frame);
	  cel->frame--;
	}
      }
      break;
    }

    case GFXOBJ_LAYER_SET: {
      JLink link;
      JI_LIST_FOR_EACH(layer->layers, link)
	remove_frame_for_layer(sprite, link->data, frame);
      break;
    }

  }
}

Command cmd_remove_frame = {
  CMD_REMOVE_FRAME,
  cmd_remove_frame_enabled,
  NULL,
  cmd_remove_frame_execute,
  NULL
};
