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

#include <assert.h>
#include <allegro/file.h>

#include "jinete/jlist.h"

#include "core/app.h"
#include "core/core.h"
#include "file/file.h"
#include "effect/effect.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "util/misc.h"
#include "widgets/editor.h"
#include "widgets/tabs.h"

/* Current selected sprite to operate, it could be not the same of
   editor_get_sprite(current_editor).  */

Sprite* current_sprite = NULL;

static JList sprites_list;
static Sprite* clipboard_sprite;

static ImageRef *images_ref_get_from_layer(Sprite* sprite, Layer *layer, int target, bool write);
static void layer_get_pos(Sprite* sprite, Layer *layer, int target, bool write, int **x, int **y, int *count);

int init_module_sprites(void)
{
  sprites_list = jlist_new();
  clipboard_sprite = NULL;
  current_sprite = NULL;
  return 0;
}

void exit_module_sprites(void)
{
  JLink link;

  if (clipboard_sprite) {
    sprite_free(clipboard_sprite);
    clipboard_sprite = NULL;
  }

  JI_LIST_FOR_EACH(sprites_list, link) {
    sprite_free(reinterpret_cast<Sprite*>(link->data));
  }
  jlist_free(sprites_list);
  sprites_list = NULL;

  current_sprite = NULL;
}

JList get_sprite_list(void)
{
  return sprites_list;
}

Sprite* get_first_sprite(void)
{
  return reinterpret_cast<Sprite*>(jlist_first_data(sprites_list));
}

Sprite* get_next_sprite(Sprite* sprite)
{
  JLink link = jlist_find(sprites_list, sprite);

  if (sprites_list->end != link &&
      sprites_list->end != link->next)
    return reinterpret_cast<Sprite*>(link->next->data);
  else
    return NULL;
}

Sprite* get_clipboard_sprite(void)
{
  return clipboard_sprite;
}

void set_clipboard_sprite(Sprite* sprite)
{
  if (clipboard_sprite) {
    if (current_sprite == clipboard_sprite)
      set_current_sprite(sprite);

    if (is_interactive())
      replace_sprite_in_editors(clipboard_sprite, sprite);

    sprite_free(clipboard_sprite);
  }

  clipboard_sprite = sprite;

  if (is_interactive())
    app_realloc_sprite_list();
}

/* adds the "sprite" in the list of sprites */
void sprite_mount(Sprite* sprite)
{
  /* append the sprite to the list */
  jlist_prepend(sprites_list, sprite);

  if (is_interactive()) {
    /* add the tab for this sprite */
    tabs_append_tab(app_get_tabsbar(),
		    get_filename(sprite->filename), sprite);

    /* rebuild the menu list of sprites */
    app_realloc_sprite_list();
  }
}

/* removes the "sprite" from the list of sprites */
void sprite_unmount(Sprite* sprite)
{
  /* remove from the sprite's list */
  jlist_remove(sprites_list, sprite);

  /* remove from the clipboard pointer */
  if (sprite == get_clipboard_sprite())
    clipboard_sprite = NULL;

  if (is_interactive()) {
    /* remove this sprite from tabs */
    tabs_remove_tab(app_get_tabsbar(), sprite);

    /* rebuild the menu list of sprites */
    app_realloc_sprite_list();

    /* select other sprites in the editors where are this sprite */
    editors_hide_sprite(sprite);
  }
  else {
    if (current_sprite == sprite)
      set_current_sprite(NULL);
  }
}

/* sets current sprite (doesn't show it, only sets the
   "current_sprite" pointer).  */
void set_current_sprite(Sprite* sprite)
{
  current_sprite = sprite;

  /* select the sprite in the tabs */
  tabs_select_tab(app_get_tabsbar(), sprite);
}

void send_sprite_to_top(Sprite* sprite)
{
  if (sprite && jlist_find(sprites_list, sprite) != sprites_list->end) {
    jlist_remove(sprites_list, sprite);
    jlist_prepend(sprites_list, sprite);
  }
}

/* puts the sprite in some editor */
void sprite_show(Sprite* sprite)
{
  if (is_interactive())
    set_sprite_in_more_reliable_editor(sprite);
}

bool is_current_sprite_not_locked(void)
{
  return
    current_sprite != NULL &&
    !sprite_is_locked(current_sprite);
}

bool is_current_sprite_writable(void)
{
  return
    current_sprite != NULL
    && !sprite_is_locked(current_sprite)
    && current_sprite->layer != NULL
    && layer_is_readable(current_sprite->layer)
    && layer_is_writable(current_sprite->layer)
    && layer_is_image(current_sprite->layer)
    && layer_get_cel(current_sprite->layer,
		     current_sprite->frame) != NULL;
}

Sprite* lock_current_sprite(void)
{
  if (current_sprite != NULL &&
      sprite_lock(current_sprite))
    return current_sprite;
  else
    return NULL;
}

ImageRef *images_ref_get_from_sprite(Sprite* sprite, int target, bool write)
{
  Layer *layer = target & TARGET_ALL_LAYERS ? sprite->set:
					      sprite->layer;

  return images_ref_get_from_layer(sprite, layer, target, write);
}

void images_ref_free(ImageRef *image_ref)
{
  ImageRef *p, *next;

  for (p=image_ref; p; p=next) {
    next = p->next;
    jfree(p);
  }
}

static ImageRef *images_ref_get_from_layer(Sprite* sprite, Layer *layer, int target, bool write)
{
#define ADD_IMAGES(images)			\
  {						\
    if (first_image == NULL) {			\
      first_image = images;			\
      last_image = images;			\
    }						\
    else {					\
      assert(last_image != NULL);		\
      last_image->next = images;		\
    }						\
						\
    while (last_image->next != NULL)		\
      last_image = last_image->next;		\
  }

#define NEW_IMAGE(layer, cel)					\
  {								\
    ImageRef *image_ref = jnew(ImageRef, 1);			\
								\
    image_ref->image = layer->sprite->stock->image[cel->image];	\
    image_ref->layer = layer;					\
    image_ref->cel = cel;					\
    image_ref->next = NULL;					\
								\
    ADD_IMAGES(image_ref);					\
  }
  
  ImageRef *first_image = NULL;
  ImageRef *last_image = NULL;
  int frame = sprite->frame;

  if (!layer_is_readable(layer))
    return NULL;

  if (write && !layer_is_writable(layer))
    return NULL;

  switch (layer->gfxobj.type) {

    case GFXOBJ_LAYER_IMAGE: {
      if (target & TARGET_ALL_FRAMES) {
	for (frame=0; frame<sprite->frames; frame++) {
	  Cel *cel = layer_get_cel(layer, frame);
	  if (cel != NULL)
	    NEW_IMAGE(layer, cel);
	}
      }
      else {
	Cel *cel = layer_get_cel(layer, frame);
	if (cel != NULL)
	  NEW_IMAGE(layer, cel);
      }
      break;
    }

    case GFXOBJ_LAYER_SET: {
      ImageRef *sub_images;
      JLink link;

      JI_LIST_FOR_EACH(layer->layers, link) {
	sub_images = images_ref_get_from_layer
	  (sprite, reinterpret_cast<Layer*>(link->data), target, write);

	if (sub_images != NULL)
	  ADD_IMAGES(sub_images);
      }
      break;
    }

  }

  return first_image;
}

static void layer_get_pos(Sprite* sprite, Layer *layer, int target, bool write, int **x, int **y, int *count)
{
  int frame = sprite->frame;

  if (!layer_is_readable(layer))
    return;

  if (write && !layer_is_writable(layer))
    return;

  switch (layer->gfxobj.type) {

    case GFXOBJ_LAYER_IMAGE: {
      Image *image;
      int u, v;

      if (target & TARGET_ALL_FRAMES) {
	for (frame=0; frame<sprite->frames; frame++) {
	  image = GetLayerImage(layer, &u, &v, frame);
	  if (image) {
	    (*x)[*count] = u;
	    (*y)[*count] = v;
	    (*count)++;
	  }
	}
      }
      else {
	image = GetLayerImage(layer, &u, &v, frame);
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
	layer_get_pos(sprite, reinterpret_cast<Layer*>(link->data), target, write, x, y, count);
      break;
    }

  }
}
