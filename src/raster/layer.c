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

#include <string.h>

#include "jinete/jlist.h"

#include "raster/blend.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "raster/stock.h"

#endif

static bool has_cels(Layer *layer, int frame);

#define LAYER_INIT(name_string)			\
  do {						\
    layer_set_name(layer, name_string);		\
						\
    layer->sprite = sprite;			\
    layer->parent = NULL;			\
    layer->readable = TRUE;			\
    layer->writable = TRUE;			\
						\
    layer->blend_mode = 0;			\
    layer->cels = NULL;				\
						\
    layer->layers = NULL;			\
  } while (0);

/**
 * Creates a new empty (without frames) normal (image) layer.
 */
Layer *layer_new(Sprite *sprite)
{
  Layer *layer = (Layer *)gfxobj_new(GFXOBJ_LAYER_IMAGE, sizeof(Layer));
  if (!layer)
    return NULL;

  LAYER_INIT("Layer");

  layer->blend_mode = BLEND_MODE_NORMAL;
  layer->cels = jlist_new();

  return layer;
}

Layer *layer_set_new(Sprite *sprite)
{
  Layer *layer = (Layer *)gfxobj_new(GFXOBJ_LAYER_SET, sizeof(Layer));
  if (!layer)
    return NULL;

  LAYER_INIT("Layer Set");

  layer->layers = jlist_new();

  return layer;
}

Layer *layer_new_copy(const Layer *layer)
{
  Layer *layer_copy = NULL;

  switch (layer->gfxobj.type) {

    case GFXOBJ_LAYER_IMAGE: {
      Cel *cel_copy;
      JLink link;

      layer_copy = layer_new(layer->sprite);
      if (!layer_copy)
	break;

      layer_set_blend_mode(layer_copy, layer->blend_mode);

      /* copy cels */
      JI_LIST_FOR_EACH(layer->cels, link) {
	cel_copy = cel_new_copy(link->data);
	if (!cel_copy) {
	  layer_free(layer_copy);
	  return NULL;
	}
	layer_add_cel(layer_copy, cel_copy);
      }
      break;
    }

    case GFXOBJ_LAYER_SET: {
      Layer *child_copy;
      JLink link;

      layer_copy = layer_set_new(layer->sprite);
      if (!layer_copy)
	break;

      JI_LIST_FOR_EACH(layer->layers, link) {
	/* copy the child */
	child_copy = layer_new_copy(link->data);
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
  if (layer_copy) {
    layer_set_name(layer_copy, layer->name);
    layer_copy->readable = layer->readable;
    layer_copy->writable = layer->writable;
  }

  return layer_copy;
}

void layer_free(Layer *layer)
{
  switch (layer->gfxobj.type) {

    case GFXOBJ_LAYER_IMAGE: {
      JLink link;

      /* remove cels */
      JI_LIST_FOR_EACH(layer->cels, link)
	cel_free(link->data);

      jlist_free(layer->cels);
      break;
    }

    case GFXOBJ_LAYER_SET: {
      JLink link;
      JI_LIST_FOR_EACH(layer->layers, link)
	layer_free(link->data);
      jlist_free(layer->layers);
      break;
    }
  }

  gfxobj_free((GfxObj *)layer);
}

/**
 * Returns TRUE if "layer" is a normal layer type (an image layer)
 */
int layer_is_image(const Layer *layer)
{
  return (layer->gfxobj.type == GFXOBJ_LAYER_IMAGE) ? TRUE: FALSE;
}

/**
 * Returns TRUE if "layer" is a set of layers
 */
int layer_is_set(const Layer *layer)
{
  return (layer->gfxobj.type == GFXOBJ_LAYER_SET) ? TRUE: FALSE;
}

/**
 * Returns TRUE if the layer is readable/viewable.
 */
bool layer_is_readable(const Layer *layer)
{
  return layer->readable;
}

/**
 * Returns TRUE if the layer is writable/editable.
 */
bool layer_is_writable(const Layer *layer)
{
  return layer->writable;
}

/**
 * Gets the previous layer of "layer" that are in the parent set.
 */
Layer *layer_get_prev(Layer *layer)
{
  if (layer->parent && layer->parent->type == GFXOBJ_LAYER_SET) {
    JList list = ((Layer *)layer->parent)->layers;
    JLink link = jlist_find(list, layer);
    if (link != list->end && link->prev != list->end)
      return link->prev->data;
  }
  return NULL;
}

Layer *layer_get_next(Layer *layer)
{
  if (layer->parent && layer->parent->type == GFXOBJ_LAYER_SET) {
    JList list = ((Layer *)layer->parent)->layers;
    JLink link = jlist_find(list, layer);
    if (link != list->end && link->next != list->end)
      return link->next->data;
  }
  return NULL;
}

void layer_set_name(Layer *layer, const char *name)
{
  strcpy(layer->name, name);
}

void layer_set_blend_mode(Layer *layer, int blend_mode)
{
  if (layer->gfxobj.type == GFXOBJ_LAYER_IMAGE)
    layer->blend_mode = blend_mode;
}

void layer_set_parent(Layer *layer, GfxObj *gfxobj)
{
  layer->parent = gfxobj;
}

void layer_add_cel(Layer *layer, Cel *cel)
{
  if (layer_is_image(layer)) {
    JLink link;

    JI_LIST_FOR_EACH(layer->cels, link)
      if (((Cel *)link->data)->frame >= cel->frame)
	break;

    jlist_insert_before(layer->cels, link, cel);
  }
}

void layer_remove_cel(Layer *layer, Cel *cel)
{
  if (layer_is_image(layer))
    jlist_remove(layer->cels, cel);
}

Cel *layer_get_cel(Layer *layer, int frame)
{
  if (layer_is_image(layer)) {
    Cel *cel;
    JLink link;

    JI_LIST_FOR_EACH(layer->cels, link) {
      cel = link->data;
      if (cel->frame == frame)
	return cel;
    }
  }

  return NULL;
}

void layer_add_layer(Layer *set, Layer *layer)
{
  if (set->gfxobj.type == GFXOBJ_LAYER_SET) {
    jlist_append(set->layers, layer);
    layer_set_parent(layer, (GfxObj *)set);
  }
}

void layer_remove_layer(Layer *set, Layer *layer)
{
  if (set->gfxobj.type == GFXOBJ_LAYER_SET) {
    jlist_remove(set->layers, layer);
    layer_set_parent(layer, NULL);
  }
}

void layer_move_layer(Layer *set, Layer *layer, Layer *after)
{
  if (set->gfxobj.type == GFXOBJ_LAYER_SET) {
    jlist_remove(set->layers, layer);

    if (after) {
      JLink before = jlist_find(set->layers, after)->next;
      jlist_insert_before(set->layers, before, layer);
    }
    else
      jlist_prepend(set->layers, layer);
  }
}

void layer_render(Layer *layer, Image *image, int x, int y, int frame)
{
  if (!layer->readable)
    return;

  switch (layer->gfxobj.type) {

    case GFXOBJ_LAYER_IMAGE: {
      Cel *cel = layer_get_cel(layer, frame);
      Image *src_image;

      if (cel) {
	if ((cel->image >= 0) &&
	    (cel->image < layer->sprite->stock->nimage))
	  src_image = layer->sprite->stock->image[cel->image];
	else
	  src_image = NULL;

	if (src_image) {
	  image_merge(image, src_image,
		      cel->x + x,
		      cel->y + y,
		      MID (0, cel->opacity, 255),
		      layer->blend_mode);
	}
      }
      break;
    }

    case GFXOBJ_LAYER_SET: {
      JLink link;
      JI_LIST_FOR_EACH(layer->layers, link)
	layer_render(link->data, image, x, y, frame);
      break;
    }

  }
}

/**
 * Returns a new layer (flat_layer) with all "layer" rendered frame
 * by frame from "frmin" to "frmax" (inclusive).  "layer" can be a set
 * of layers, so the routines flatten all childs to the unique output
 * layer.
 */
Layer *layer_flatten(Layer *layer, int x, int y, int w, int h, int frmin, int frmax)
{
  Layer *flat_layer;
  Image *image;
  Cel *cel;
  int frame;

  flat_layer = layer_new(layer->sprite);
  if (!flat_layer)
    return NULL;

  layer_set_name(flat_layer, "Flat Layer");

  for (frame=frmin; frame<=frmax; frame++) {
    /* does this frame have cels to render? */
    if (has_cels(layer, frame)) {
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
      layer_render(layer, image, -x, -y, frame);
      layer_add_cel(flat_layer, cel);
    }
  }

  return flat_layer;
}

/**
 * Returns TRUE if the "layer" (or him childs) has cels to render in
 * frame.
 */
static bool has_cels(Layer *layer, int frame)
{
  if (!layer->readable)
    return FALSE;

  switch (layer->gfxobj.type) {

    case GFXOBJ_LAYER_IMAGE:
      return layer_get_cel(layer, frame) ? TRUE: FALSE;

    case GFXOBJ_LAYER_SET: {
      JLink link;
      JI_LIST_FOR_EACH(layer->layers, link) {
	if (has_cels(link->data, frame))
	  return TRUE;
      }
      break;
    }

  }

  return FALSE;
}
