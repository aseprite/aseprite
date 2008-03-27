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

#include <allegro/keyboard.h>

#include "core/app.h"
#include "core/cfg.h"
#include "core/core.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "modules/tools2.h"
#include "raster/blend.h"
#include "raster/cel.h"
#include "raster/dirty.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "raster/undo.h"
#include "util/misc.h"
#include "widgets/colbar.h"

enum { ACTION_MOVE, ACTION_COPY, ACTION_SWAP };

static Layer *handle_layer;

static void do_quick(int action);
static int my_callback(void);

void quick_move(void) { do_quick(ACTION_MOVE); }
void quick_copy(void) { do_quick(ACTION_COPY); }
void quick_swap(void) { do_quick(ACTION_SWAP); }

static void do_quick(int action)
{
  Sprite *sprite = current_sprite;
  Layer *dst_layer;
  int x, y, ret;
  Dirty *dirty;
  Image *dst;
  Mask *mask_backup;

  if (!is_interactive() || !sprite)
    return;

  dst = GetImage2(sprite, &x, &y, NULL);
  if (!dst)
    return;

  /* get the current layer */
  dst_layer = sprite->layer;

  /* create a new layer from the mask */
  handle_layer = NewLayerFromMask(sprite, sprite);
  if (!handle_layer)
    return;

  if (get_config_bool ("QuickMovement", "UseMask", TRUE))
    layer_set_blend_mode(handle_layer, BLEND_MODE_NORMAL);
  else
    layer_set_blend_mode(handle_layer, BLEND_MODE_COPY);

  /* create a new dirty */
  dirty = dirty_new(dst, 0, 0, dst->w-1, dst->h-1, FALSE);

  /* save the current mask region */
  if (action != ACTION_COPY) {
    dirty_rectfill(dirty,
		   sprite->mask->x-x,
		   sprite->mask->y-y,
		   sprite->mask->x-x + sprite->mask->w-1,
		   sprite->mask->y-y + sprite->mask->h-1);

    dirty_save_image_data(dirty);
  }

  /* clear the mask region */
  if (action == ACTION_MOVE) {
    int enabled = undo_is_enabled(sprite->undo);
    undo_disable(sprite->undo);
    ClearMask();
    if (enabled)
      undo_enable(sprite->undo);
  }

  /* copy the mask */
  mask_backup = mask_new_copy(sprite->mask);

  /* deselect the mask */
  mask_none(sprite->mask);

  /* insert the new layer in the top of the current one */
  layer_add_layer(sprite->set, handle_layer);
  layer_move_layer(sprite->set, handle_layer, dst_layer);

  /* select the layer */
  sprite_set_layer(sprite, handle_layer);

  /* regenerate the boundaries(the mask was deselected) and redraw
     the sprite */
  sprite_generate_mask_boundaries(sprite);
  update_screen_for_sprite(sprite);

  /* move the new layer to a new position */
  ret = interactive_move_layer(MODE_CLICKANDCLICK, FALSE, my_callback);

  /* all ok, merge the layer down */
  if (ret) {
    int u, v;
    Image *src = GetImage2(sprite, &u, &v, NULL);
    Dirty *dirty_copy = dirty_new_copy(dirty);

    /* restore the "dst" image */
    dirty_swap(dirty_copy);

    /* save the mask area in the new position too */
    dirty_rectfill(dirty, u-x, v-y,
		   u-x + src->w-1,
		   v-y + src->h-1);
    dirty_save_image_data(dirty);

    /* put the first cleared part */
    dirty_restore_image_data(dirty_copy);
    dirty_free(dirty_copy);

    switch (action) {

      case ACTION_MOVE:
      case ACTION_COPY:
	/* merge the source image in the destination */
	image_merge(dst, src, u-x, v-y, 255, handle_layer->blend_mode);
	break;

      case ACTION_SWAP:
	/* swap areas */
	/* sprite_set_mask(sprite, mask_backup); */
	/* image_swap(dst, src, u-x, v-y); */
	break;
    }

    /* restore the mask */
    sprite_set_mask(sprite, mask_backup);

    /* insert the undo operation */
    if (undo_is_enabled (sprite->undo)) {
      undo_open(sprite->undo);
      undo_dirty(sprite->undo, dirty);
      undo_int(sprite->undo, (GfxObj *)sprite->mask, &sprite->mask->x);
      undo_int(sprite->undo, (GfxObj *)sprite->mask, &sprite->mask->y);
      undo_close(sprite->undo);
    }

    /* move the mask to the new position */
    mask_move(sprite->mask, u-(sprite->mask->x-x), v-(sprite->mask->y-y));
    sprite_generate_mask_boundaries(sprite);
  }
  /* user cancels the operation */
  else {
    /* restore the "dst" image */
    dirty_restore_image_data(dirty);

    /* restore the mask */
    sprite_set_mask(sprite, mask_backup);
    sprite_generate_mask_boundaries(sprite);
  }

  /* free the mask copy */
  mask_free(mask_backup);

  /* free the dirty */
  dirty_free(dirty);

  /* select the destination layer */
  sprite_set_layer(sprite, dst_layer);

  /* remove the temporary created layer */
  layer_remove_layer(handle_layer->parent_layer, handle_layer);

  /* refresh the sprite */
  update_screen_for_sprite(sprite);
}

static int my_callback(void)
{
  int ret = FALSE;

  if (keypressed()) {
    int scancode = (readkey()>>8);

    if (scancode == KEY_M) {
      ret = TRUE;

      /* switch mask status */
      if (get_config_bool("QuickMovement", "UseMask", TRUE))
	set_config_bool("QuickMovement", "UseMask", FALSE);
      else
	set_config_bool("QuickMovement", "UseMask", TRUE);

      /* set layer blend mode */
      if (get_config_bool("QuickMovement", "UseMask", TRUE))
	layer_set_blend_mode(handle_layer, BLEND_MODE_NORMAL);
      else
	layer_set_blend_mode(handle_layer, BLEND_MODE_COPY);
    }
  }

  return ret;
}
