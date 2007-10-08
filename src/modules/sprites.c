/* ase -- allegro-sprite-editor: the ultimate sprites factory
 * Copyright (C) 2001-2005, 2007  David A. Capello
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

#include "jinete/list.h"

#include "core/core.h"
#include "file/file.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "script/bindings.h"
#include "util/misc.h"
#include "widgets/editor.h"

#endif

/* Current selected sprite to operate, it could be not the same of
   editor_get_sprite (current_editor).  */

Sprite *current_sprite = NULL;

static JList sprites_list;
static Sprite *clipboard_sprite;

static Stock *layer_get_images (Sprite *sprite, Layer *layer, int target, int write);
static void layer_get_pos (Sprite *sprite, Layer *layer, int target, int write, int **x, int **y, int *count);

int init_module_sprites(void)
{
  sprites_list = jlist_new();
  clipboard_sprite = NULL;
  current_sprite = NULL;
  return 0;
}

void exit_module_sprites(void)
{
  if (clipboard_sprite) {
    sprite_free(clipboard_sprite);
    clipboard_sprite = NULL;
  }

  jlist_free(sprites_list);
  sprites_list = NULL;

  current_sprite = NULL;
}

JList get_sprite_list(void)
{
  return sprites_list;
}

Sprite *get_first_sprite(void)
{
  return jlist_first_data(sprites_list);
}

Sprite *get_next_sprite(Sprite *sprite)
{
  JLink link = jlist_find(sprites_list, sprite);

  if (sprites_list->end != link &&
      sprites_list->end != link->next)
    return link->next->data;
  else
    return NULL;
}

Sprite *get_clipboard_sprite(void)
{
  return clipboard_sprite;
}

void set_clipboard_sprite(Sprite *sprite)
{
  if (clipboard_sprite) {
    if (current_sprite == clipboard_sprite)
      set_current_sprite (sprite);

    if (is_interactive ())
      replace_sprite_in_editors (clipboard_sprite, sprite);

    sprite_free (clipboard_sprite);
  }

  clipboard_sprite = sprite;

  if (is_interactive ())
    rebuild_sprite_list ();
}

/* adds the "sprite" in the list of sprites */
void sprite_mount(Sprite *sprite)
{
  /* append the sprite to the list */
  jlist_prepend(sprites_list, sprite);

  /* rebuild the menu list of sprites */
  if (is_interactive())
    rebuild_sprite_list();
}

/* removes the "sprite" from the list of sprites */
void sprite_unmount(Sprite *sprite)
{
  /* remove from the sprite's list */
  jlist_remove(sprites_list, sprite);

  /* remove from the clipboard pointer */
  if (sprite == get_clipboard_sprite())
    clipboard_sprite = NULL;

  if (is_interactive()) {
    /* rebuild the menu list of sprites */
    rebuild_sprite_list();

    /* select other sprites in the editors where are this sprite */
    editors_hide_sprite(sprite);
  }
  else {
    if (current_sprite == sprite)
      set_current_sprite(NULL);
  }
}

/* closes the specified sprite */
void sprite_close(Sprite *sprite)
{
  sprite_unmount(sprite);
  sprite_free(sprite);
}

/* sets current sprite (doesn't show it, only sets the
   "current_sprite" pointer).  */
void set_current_sprite(Sprite *sprite)
{
  current_sprite = sprite;
  update_global_script_variables();
}

void send_sprite_to_top(Sprite *sprite)
{
  if (sprite && jlist_find(sprites_list, sprite) != sprites_list->end) {
    jlist_remove(sprites_list, sprite);
    jlist_prepend(sprites_list, sprite);
  }
}

/* puts the sprite in some editor */
void sprite_show(Sprite *sprite)
{
  if (is_interactive())
    set_sprite_in_more_reliable_editor(sprite);
}

Stock *sprite_get_images (Sprite *sprite, int target, int write, int **x, int **y)
{
  Layer *layer = (target & TARGET_LAYERS) ? sprite->set: sprite->layer;
  Stock *stock;

  stock = layer_get_images(sprite, layer, target, write);

  if (x && y && stock->nimage > 0) {
    int count = 0;

    *x = jmalloc(sizeof(int) * stock->nimage);
    *y = jmalloc(sizeof(int) * stock->nimage);

    /* first stock image is a NULL image (doesn't have position) */
    (*x)[count] = 0;
    (*y)[count] = 0;
    count++;

    layer_get_pos(sprite, layer, target, write, x, y, &count);
  }

  return stock;
}

static Stock *layer_get_images(Sprite *sprite, Layer *layer, int target, int write)
{
  Stock *stock = stock_new_ref(sprite->imgtype);
  int frpos = sprite->frpos;

  if (!layer->readable)
    return stock;

  if (write && !layer->writable)
    return stock;

  switch (layer->gfxobj.type) {

    case GFXOBJ_LAYER_IMAGE: {
      Image *image;

      if (target & TARGET_FRAMES) {
	for (frpos=0; frpos<sprite->frames; frpos++) {
	  image = GetLayerImage(layer, NULL, NULL, frpos);
	  if (image)
	    stock_add_image(stock, image);
	}
      }
      else {
	image = GetLayerImage(layer, NULL, NULL, frpos);
	if (image)
	  stock_add_image(stock, image);
      }
      break;
    }

    case GFXOBJ_LAYER_SET: {
      Stock *sub_stock;
      JLink link;
      int c;

      JI_LIST_FOR_EACH(layer->layers, link) {
	sub_stock = layer_get_images(sprite, link->data, target, write);

	if (sub_stock) {
	  for (c=0; c<sub_stock->nimage; c++) {
	    if (sub_stock->image[c])
	      stock_add_image (stock, sub_stock->image[c]);
	  }
	  stock_free (sub_stock);
	}
      }
      break;
    }

    case GFXOBJ_LAYER_TEXT:
      /* XXX */
      break;
  }

  return stock;
}

static void layer_get_pos (Sprite *sprite, Layer *layer, int target, int write, int **x, int **y, int *count)
{
  int frpos = sprite->frpos;

  if (!layer->readable)
    return;

  if (write && !layer->writable)
    return;

  switch (layer->gfxobj.type) {

    case GFXOBJ_LAYER_IMAGE: {
      Image *image;
      int u, v;

      if (target & TARGET_FRAMES) {
	for (frpos=0; frpos<sprite->frames; frpos++) {
	  image = GetLayerImage (layer, &u, &v, frpos);
	  if (image) {
	    (*x)[*count] = u;
	    (*y)[*count] = v;
	    (*count)++;
	  }
	}
      }
      else {
	image = GetLayerImage (layer, &u, &v, frpos);
	if (image) {
	  (*x)[*count] = u;
	  (*y)[*count] = v;
	  (*count)++;
	}
      }
      break;
    }

    case GFXOBJ_LAYER_SET: {
      JLink link;
      JI_LIST_FOR_EACH(layer->layers, link)
	layer_get_pos(sprite, link->data, target, write, x, y, count);
      break;
    }

    case GFXOBJ_LAYER_TEXT:
      /* XXX */
      break;
  }
}
