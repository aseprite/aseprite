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

#include <string.h>

#include "jinete/list.h"

#include "raster/blend.h"
#include "raster/frame.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/stock.h"

#endif

static int has_frames (Layer *layer, int frpos);

#define LAYER_INIT(name_string)			\
  do {						\
    layer_set_name(layer, name_string);		\
						\
    layer->parent = NULL;			\
    layer->readable = TRUE;			\
    layer->writable = TRUE;			\
						\
    layer->imgtype = 0;				\
    layer->blend_mode = 0;			\
    layer->stock = NULL;			\
    layer->frames = NULL;			\
						\
    layer->layers = NULL;			\
  } while (0);

/* creates a new empty (without frames) normal (image) layer */
Layer *layer_new(int imgtype)
{
  Layer *layer = (Layer *)gfxobj_new(GFXOBJ_LAYER_IMAGE, sizeof(Layer));
  if (!layer)
    return NULL;

  LAYER_INIT("Layer");

  layer->imgtype = imgtype;
  layer->blend_mode = BLEND_MODE_NORMAL;
  layer->stock = stock_new(imgtype);
  layer->frames = jlist_new();

  return layer;
}

Layer *layer_set_new(void)
{
  Layer *layer = (Layer *)gfxobj_new(GFXOBJ_LAYER_SET, sizeof(Layer));
  if (!layer)
    return NULL;

  LAYER_INIT("Layer Set");

  layer->layers = jlist_new();

  return layer;
}

#if 0				/* XXXX */
Layer *layer_text_new (const char *text)
{
  Layer *layer;

  layer = jnew (Layer, 1);
  if (!layer)
    return NULL;

  LAYER_INIT (GFXOBJ_LAYER_TEXT, "Layer Text");

  /* XXX */
  /* layer->font = NULL; */
/*   layer->text = text ? jstrdup (text): NULL; */
/*   layer->pos_x = prop_new ("Pos X"); */
/*   layer->pos_y = prop_new ("Pos Y"); */
/*   layer->opacity = prop_new ("Opacity"); */
/*   layer->blend_mode = BLEND_MODE_NORMAL; */

  return layer;
}
#endif

Layer *layer_new_copy(const Layer *layer)
{
  Layer *layer_copy = NULL;

  switch (layer->gfxobj.type) {

    case GFXOBJ_LAYER_IMAGE: {
      Frame *frame_copy;
      JLink link;

      layer_copy = layer_new(layer->imgtype);
      if (!layer_copy)
	break;

      layer_set_blend_mode(layer_copy, layer->blend_mode);

      /* copy stock */
      stock_free(layer_copy->stock);
      layer_copy->stock = stock_new_copy(layer->stock);
      if (!layer_copy->stock) {
	layer_free (layer_copy);
	return NULL;
      }

      /* copy frames */
      JI_LIST_FOR_EACH(layer->frames, link) {
	frame_copy = frame_new_copy(link->data);
	if (!frame_copy) {
	  layer_free(layer_copy);
	  return NULL;
	}
	layer_add_frame(layer_copy, frame_copy);
      }
      break;
    }

    case GFXOBJ_LAYER_SET: {
      Layer *child_copy;
      JLink link;

      layer_copy = layer_set_new();
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

    case GFXOBJ_LAYER_TEXT:
      /* XXX */
      break;
  }

  /* copy general properties */
  if (layer_copy) {
    layer_set_name(layer_copy, layer->name);
    layer_copy->readable = layer->readable;
    layer_copy->writable = layer->writable;
  }

  return layer_copy;
}

Layer *layer_new_with_image(int imgtype, int x, int y, int w, int h, int frpos)
{
  Layer *layer;
  Frame *frame;
  Image *image;
  int index;

  /* new image */
  image = image_new(imgtype, w, h);
  if (!image)
    return NULL;

  /* new layer */
  layer = layer_new(imgtype);
  if (!layer) {
    image_free (image);
    return NULL;
  }

  /* clear with mask color */
  image_clear(image, 0);

  /* configure layer name and blend mode */
  layer_set_name (layer, "Background");
  layer_set_blend_mode (layer, BLEND_MODE_NORMAL);

  /* add image in the layer stock */
  index = stock_add_image(layer->stock, image);

  /* create the frame */
  frame = frame_new(frpos, index);
  frame_set_position (frame, x, y);

  /* add the frame in the layer */
  layer_add_frame(layer, frame);

  return layer;
}

void layer_free (Layer *layer)
{
  switch (layer->gfxobj.type) {

    case GFXOBJ_LAYER_IMAGE: {
      JLink link;

      /* destroy images' stock */
      if (layer->stock)
	stock_free(layer->stock);

      /* remove frames */
      JI_LIST_FOR_EACH(layer->frames, link)
	frame_free(link->data);

      jlist_free(layer->frames);
      break;
    }

    case GFXOBJ_LAYER_SET: {
      JLink link;
      JI_LIST_FOR_EACH(layer->layers, link)
	layer_free(link->data);
      break;
    }

    case GFXOBJ_LAYER_TEXT:
/*       if (layer->text) */
/* 	r_free (layer->text); */

/*       prop_free (layer->pos_x); */
/*       prop_free (layer->pos_y); */
/*       prop_free (layer->opacity); */
      break;
  }

  gfxobj_free((GfxObj *)layer);
}

/* returns TRUE if "layer" is a normal layer type (an image layer)  */
int layer_is_image (const Layer *layer)
{
  return (layer->gfxobj.type == GFXOBJ_LAYER_IMAGE) ? TRUE: FALSE;
}

/* returns TRUE if "layer" is a set of layers */
int layer_is_set (const Layer *layer)
{
  return (layer->gfxobj.type == GFXOBJ_LAYER_SET) ? TRUE: FALSE;
}

/* gets the previous layer of "layer" that are in the parent set */
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

void layer_set_name (Layer *layer, const char *name)
{
  strcpy (layer->name, name);
}

void layer_set_blend_mode (Layer *layer, int blend_mode)
{
  if (layer->gfxobj.type == GFXOBJ_LAYER_IMAGE)
    layer->blend_mode = blend_mode;
}

void layer_set_parent (Layer *layer, GfxObj *gfxobj)
{
  layer->parent = gfxobj;
}

void layer_add_frame(Layer *layer, Frame *frame)
{
  if (layer_is_image(layer)) {
    JLink link;

    JI_LIST_FOR_EACH(layer->frames, link)
      if (((Frame *)link->data)->frpos >= frame->frpos)
	break;

    jlist_insert_before(layer->frames, link, frame);
  }
}

void layer_remove_frame(Layer *layer, Frame *frame)
{
  if (layer_is_image(layer))
    jlist_remove(layer->frames, frame);
}

Frame *layer_get_frame(Layer *layer, int frpos)
{
  if (layer_is_image(layer)) {
    Frame *frame;
    JLink link;

    JI_LIST_FOR_EACH(layer->frames, link) {
      frame = link->data;
      if (frame->frpos == frpos)
	return frame;
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

void layer_remove_layer (Layer *set, Layer *layer)
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

void layer_render(Layer *layer, Image *image, int x, int y, int frpos)
{
  if (!layer->readable)
    return;

  switch (layer->gfxobj.type) {

    case GFXOBJ_LAYER_IMAGE: {
      Frame *frame = layer_get_frame (layer, frpos);
      Image *src_image;

      if (frame) {
	if ((frame->image >= 0) &&
	    (frame->image < layer->stock->nimage))
	  src_image = layer->stock->image[frame->image];
	else
	  src_image = NULL;

	if (src_image) {
	  image_merge(image, src_image,
		      frame->x + x,
		      frame->y + y,
		      MID (0, frame->opacity, 255),
		      layer->blend_mode);
	}
      }
      break;
    }

    case GFXOBJ_LAYER_SET: {
      JLink link;
      JI_LIST_FOR_EACH(layer->layers, link)
	layer_render(link->data, image, x, y, frpos);
      break;
    }

    case GFXOBJ_LAYER_TEXT:
      /* XXX */
      break;
  }
}

/* returns a new layer (flat_layer) with all "layer" rendered frames
   by frame from "frmin" to "frmax" (inclusive).  "layer" can be a set
   of layers, so the routines flatten all childs to the unique output
   layer */
Layer *layer_flatten(Layer *layer, int imgtype,
		     int x, int y, int w, int h, int frmin, int frmax)
{
  Layer *flat_layer;
  Image *image;
  Frame *frame;
  int frpos;

  flat_layer = layer_new(imgtype);
  if (!flat_layer)
    return NULL;

  layer_set_name(flat_layer, "Flat Layer");

  for (frpos=frmin; frpos<=frmax; frpos++) {
    /* this "frpos" has frames to render */
    if (has_frames(layer, frpos)) {
      /* create a new image */
      image = image_new(imgtype, w, h);
      if (!image) {
	layer_free(flat_layer);
	return NULL;
      }

      /* create the new frame for the output layer (add the image to
	 stock too) */
      frame = frame_new(frpos, stock_add_image(flat_layer->stock, image));
      frame_set_position(frame, x, y);
      if (!frame) {
	layer_free(flat_layer);
	image_free(image);
	return NULL;
      }

      /* clear the image and render this frame */
      image_clear(image, 0);
      layer_render(layer, image, -x, -y, frpos);
      layer_add_frame(flat_layer, frame);
    }
  }

  return flat_layer;
}

/* returns TRUE if the "layer" (or him childs) has frames to render in frpos */
static int has_frames(Layer *layer, int frpos)
{
  if (!layer->readable)
    return FALSE;

  switch (layer->gfxobj.type) {

    case GFXOBJ_LAYER_IMAGE:
      return layer_get_frame(layer, frpos) ? TRUE: FALSE;

    case GFXOBJ_LAYER_SET: {
      JLink link;
      JI_LIST_FOR_EACH(layer->layers, link)
	if (has_frames(link->data, frpos))
	  return TRUE;
      break;
    }

    case GFXOBJ_LAYER_TEXT:
      /* XXX */
      break;
  }

  return FALSE;
}
