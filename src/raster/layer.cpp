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

#include <assert.h>
#include <string.h>
#include <allegro/unicode.h>

#include "jinete/jlist.h"

#include "raster/blend.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "raster/undo.h"

static bool has_cels(const Layer* layer, int frame);
static void layer_set_parent(Layer* layer, Layer* parent_set);

//////////////////////////////////////////////////////////////////////

Layer::Layer(int type, Sprite* sprite)
  : GfxObj(type)
{
  assert(type == GFXOBJ_LAYER_IMAGE || type == GFXOBJ_LAYER_SET);

  layer_set_name(this, "");

  this->sprite = sprite;
  this->parent_layer = NULL;
  this->flags =
    LAYER_IS_READABLE |
    LAYER_IS_WRITABLE;
  
  this->blend_mode = 0;
  this->cels = NULL;

  this->layers = NULL;
}

Layer::~Layer()
{
  switch (this->type) {

    case GFXOBJ_LAYER_IMAGE: {
      JLink link;

      /* remove cels */
      JI_LIST_FOR_EACH(this->cels, link)
	cel_free(reinterpret_cast<Cel*>(link->data));

      jlist_free(this->cels);
      break;
    }

    case GFXOBJ_LAYER_SET: {
      JLink link;
      JI_LIST_FOR_EACH(this->layers, link)
	layer_free(reinterpret_cast<Layer*>(link->data));
      jlist_free(this->layers);
      break;
    }
  }
}

//////////////////////////////////////////////////////////////////////

/**
 * Creates a new empty (without frames) normal (image) layer.
 */
Layer* layer_new(Sprite* sprite)
{
  Layer* layer = new Layer(GFXOBJ_LAYER_IMAGE, sprite);

  layer_set_name(layer, "Layer");

  layer->blend_mode = BLEND_MODE_NORMAL;
  layer->cels = jlist_new();

  return layer;
}

Layer* layer_set_new(Sprite *sprite)
{
  Layer* layer = new Layer(GFXOBJ_LAYER_SET, sprite);

  layer_set_name(layer, "Layer Set");

  layer->layers = jlist_new();

  return layer;
}

Layer* layer_new_copy(Sprite *dst_sprite, const Layer* src_layer)
{
  Layer* layer_copy = NULL;

  assert(dst_sprite != NULL);
  
  switch (src_layer->type) {

    case GFXOBJ_LAYER_IMAGE: {
      Cel* cel_copy, *cel;
      Image* image_copy, *image;
      JLink link;

      layer_copy = layer_new(dst_sprite);
      if (!layer_copy)
	break;

      layer_set_blend_mode(layer_copy, src_layer->blend_mode);

      /* copy cels */
      JI_LIST_FOR_EACH(src_layer->cels, link) {
	cel = (Cel* )link->data;
	cel_copy = cel_new_copy(cel);
	if (!cel_copy) {
	  layer_free(layer_copy);
	  return NULL;
	}

	assert((cel->image >= 0) &&
	       (cel->image < src_layer->sprite->stock->nimage));

	image = src_layer->sprite->stock->image[cel->image];
	assert(image != NULL);

	image_copy = image_new_copy(image);

	cel_copy->image = stock_add_image(dst_sprite->stock, image_copy);
	if (undo_is_enabled(dst_sprite->undo))
	  undo_add_image(dst_sprite->undo, dst_sprite->stock, cel_copy->image);

	layer_add_cel(layer_copy, cel_copy);
      }
      break;
    }

    case GFXOBJ_LAYER_SET: {
      Layer* child_copy;
      JLink link;

      layer_copy = layer_set_new(dst_sprite);
      if (!layer_copy)
	break;

      JI_LIST_FOR_EACH(src_layer->layers, link) {
	/* copy the child */
	child_copy = layer_new_copy(dst_sprite,
				    reinterpret_cast<Layer*>(link->data));
	/* not enough memory? */
	if (!child_copy) {
	  layer_free(layer_copy);
	  return NULL;
	}

	/* add the new child in the layer copy */
	layer_add_layer(layer_copy, child_copy);
      }
      break;
    }

  }

  /* copy general properties */
  if (layer_copy != NULL) {
    layer_set_name(layer_copy, src_layer->name);
    layer_copy->flags = src_layer->flags;
  }

  return layer_copy;
}

/**
 * Returns a new layer (flat_layer) with all "layer" rendered frame by
 * frame from "frmin" to "frmax" (inclusive).  "layer" can be a set of
 * layers, so the routine flattens all children to an unique output
 * layer.
 *
 * @param dst_sprite The sprite where to put the new flattened layer.
 * @param src_layer Generally a set of layers to be flattened.
 */
Layer* layer_new_flatten_copy(Sprite *dst_sprite, const Layer* src_layer,
			      int x, int y, int w, int h, int frmin, int frmax)
{
  Layer* flat_layer;
  Image* image;
  Cel* cel;
  int frame;

  flat_layer = layer_new(dst_sprite);
  if (!flat_layer)
    return NULL;

  for (frame=frmin; frame<=frmax; frame++) {
    /* does this frame have cels to render? */
    if (has_cels(src_layer, frame)) {
      /* create a new image */
      image = image_new(flat_layer->sprite->imgtype, w, h);
      if (!image) {
	layer_free(flat_layer);
	return NULL;
      }

      /* create the new cel for the output layer (add the image to
	 stock too) */
      cel = cel_new(frame, stock_add_image(flat_layer->sprite->stock, image));
      cel_set_position(cel, x, y);
      if (!cel) {
	layer_free(flat_layer);
	image_free(image);
	return NULL;
      }

      /* clear the image and render this frame */
      image_clear(image, 0);
      layer_render(src_layer, image, -x, -y, frame);
      layer_add_cel(flat_layer, cel);
    }
  }

  return flat_layer;
}

void layer_free(Layer* layer)
{
  assert(layer);
  delete layer;
}


void layer_free_images(Layer* layer)
{
  JLink link;

  switch (layer->type) {

    case GFXOBJ_LAYER_IMAGE:
      JI_LIST_FOR_EACH(layer->cels, link) {
	Cel* cel = reinterpret_cast<Cel*>(link->data);

	if (!cel_is_link(cel, layer)) {
	  Image* image = layer->sprite->stock->image[cel->image];

	  assert(image != NULL);

	  stock_remove_image(layer->sprite->stock, image);
	  image_free(image);
	}
      }
      break;

    case GFXOBJ_LAYER_SET: {
      JI_LIST_FOR_EACH(layer->layers, link)
	layer_free_images(reinterpret_cast<Layer*>(link->data));
      break;
    }
  }
}

/**
 * Configures some properties of the specified layer to make it as the
 * "Background" of the sprite.
 *
 * You can't use this routine if the sprite already has a background
 * layer.
 */
void layer_configure_as_background(Layer* layer)
{
  assert(layer != NULL);
  assert(layer->sprite != NULL);
  assert(sprite_get_background_layer(layer->sprite) == NULL);

  layer->flags |= LAYER_IS_LOCKMOVE | LAYER_IS_BACKGROUND;
  layer_set_name(layer, "Background");

  layer_move_layer(layer->sprite->set, layer, NULL);
}

/**
 * Returns TRUE if "layer" is a normal layer type (an image layer)
 */
bool layer_is_image(const Layer* layer)
{
  return (layer->type == GFXOBJ_LAYER_IMAGE) ? TRUE: FALSE;
}

/**
 * Returns TRUE if "layer" is a set of layers
 */
bool layer_is_set(const Layer* layer)
{
  return (layer->type == GFXOBJ_LAYER_SET) ? TRUE: FALSE;
}

/**
 * Returns TRUE if the layer is readable/viewable.
 */
bool layer_is_readable(const Layer* layer)
{
  return (layer->flags & LAYER_IS_READABLE) == LAYER_IS_READABLE;
}

/**
 * Returns TRUE if the layer is writable/editable.
 */
bool layer_is_writable(const Layer* layer)
{
  return (layer->flags & LAYER_IS_WRITABLE) == LAYER_IS_WRITABLE;
}

/**
 * Returns TRUE if the layer is moveable.
 */
bool layer_is_moveable(const Layer* layer)
{
  return (layer->flags & LAYER_IS_LOCKMOVE) == 0;
}

/**
 * Returns TRUE if the layer is the background.
 */
bool layer_is_background(const Layer* layer)
{
  return (layer->flags & LAYER_IS_BACKGROUND) == LAYER_IS_BACKGROUND;
}

/**
 * Gets the previous layer of "layer" that are in the parent set.
 */
Layer* layer_get_prev(Layer* layer)
{
  if (layer->parent_layer != NULL) {
    JList list = layer->parent_layer->layers;
    JLink link = jlist_find(list, layer);
    if (link != list->end && link->prev != list->end)
      return reinterpret_cast<Layer*>(link->prev->data);
  }
  return NULL;
}

Layer* layer_get_next(Layer* layer)
{
  if (layer->parent_layer != NULL) {
    JList list = layer->parent_layer->layers;
    JLink link = jlist_find(list, layer);
    if (link != list->end && link->next != list->end)
      return reinterpret_cast<Layer*>(link->next->data);
  }
  return NULL;
}

void layer_set_name(Layer* layer, const char *name)
{
  ustrzcpy(layer->name, LAYER_NAME_SIZE, name);
}

void layer_set_blend_mode(Layer* layer, int blend_mode)
{
  if (layer_is_image(layer))
    layer->blend_mode = blend_mode;
}

void layer_add_cel(Layer* layer, Cel* cel)
{
  if (layer_is_image(layer)) {
    JLink link;

    JI_LIST_FOR_EACH(layer->cels, link)
      if (((Cel* )link->data)->frame >= cel->frame)
	break;

    jlist_insert_before(layer->cels, link, cel);
  }
}

/**
 * Removes the cel from the layer.
 *
 * It doesn't destroy the cel, you have to delete it after calling
 * this routine.
 */
void layer_remove_cel(Layer* layer, Cel* cel)
{
  assert(layer_is_image(layer));
  jlist_remove(layer->cels, cel);
}

Cel* layer_get_cel(const Layer* layer, int frame)
{
  if (layer_is_image(layer)) {
    Cel* cel;
    JLink link;

    JI_LIST_FOR_EACH(layer->cels, link) {
      cel = reinterpret_cast<Cel*>(link->data);
      if (cel->frame == frame)
	return cel;
    }
  }

  return NULL;
}

void layer_add_layer(Layer* set, Layer* layer)
{
  assert(set != NULL && layer_is_set(set));

  jlist_append(set->layers, layer);
  layer_set_parent(layer, set);
}

void layer_remove_layer(Layer* set, Layer* layer)
{
  assert(set != NULL && layer_is_set(set));

  jlist_remove(set->layers, layer);
  layer_set_parent(layer, NULL);
}

void layer_move_layer(Layer* set, Layer* layer, Layer* after)
{
  assert(set != NULL && layer_is_set(set));

  jlist_remove(set->layers, layer);

  if (after) {
    JLink before = jlist_find(set->layers, after)->next;
    jlist_insert_before(set->layers, before, layer);
  }
  else
    jlist_prepend(set->layers, layer);
}

void layer_render(const Layer* layer, Image* image, int x, int y, int frame)
{
  if (!layer_is_readable(layer))
    return;

  switch (layer->type) {

    case GFXOBJ_LAYER_IMAGE: {
      Cel* cel = layer_get_cel(layer, frame);
      Image* src_image;

      if (cel) {
	assert((cel->image >= 0) &&
	       (cel->image < layer->sprite->stock->nimage));

	src_image = layer->sprite->stock->image[cel->image];
	assert(src_image != NULL);

	image_merge(image, src_image,
		    cel->x + x,
		    cel->y + y,
		    MID (0, cel->opacity, 255),
		    layer->blend_mode);
      }
      break;
    }

    case GFXOBJ_LAYER_SET: {
      JLink link;
      JI_LIST_FOR_EACH(layer->layers, link)
	layer_render(reinterpret_cast<Layer*>(link->data), image, x, y, frame);
      break;
    }

  }
}

/**
 * Returns TRUE if the "layer" (or him childs) has cels to render in
 * frame.
 */
static bool has_cels(const Layer* layer, int frame)
{
  if (!layer_is_readable(layer))
    return FALSE;

  switch (layer->type) {

    case GFXOBJ_LAYER_IMAGE:
      return layer_get_cel(layer, frame) ? TRUE: FALSE;

    case GFXOBJ_LAYER_SET: {
      JLink link;
      JI_LIST_FOR_EACH(layer->layers, link) {
	if (has_cels(reinterpret_cast<const Layer*>(link->data), frame))
	  return TRUE;
      }
      break;
    }

  }

  return FALSE;
}

static void layer_set_parent(Layer* layer, Layer* parent_set)
{
  assert(parent_set == NULL || layer_is_set(parent_set));

  layer->parent_layer = parent_set;
}
