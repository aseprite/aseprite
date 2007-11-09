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

#include "jinete.h"

#include "console/console.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/blend.h"
#include "raster/image.h"
#include "raster/frame.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "raster/undo.h"

#endif

/* ======================================= */
/* Layer                                   */
/* ======================================= */

/* internal routine */
static int count_layers(Layer *layer)
{
  int count;

  if (layer->parent->type == GFXOBJ_SPRITE)
    count = 0;
  else
    count = 1;

  if (layer_is_set(layer)) {
    JLink link;
    JI_LIST_FOR_EACH(layer->layers, link) {
      count += count_layers(link->data);
    }
  }

  return count;
}

char *GetUniqueLayerName(void)
{
  Sprite *sprite = current_sprite;
  if (sprite) {
    char buf[1024];
    sprintf(buf, "%s %d", _("Layer"), count_layers(sprite->set));
    return jstrdup(buf);
  }
  else
    return NULL;
}

/**
 * Creates a new layer with the "name" in the current sprite(in the
 * current frame) with the specified position and size(if w=h=0 the
 * routine will use the sprite dimension)
 */
Layer *NewLayer(const char *name, int x, int y, int w, int h)
{
  Sprite *sprite = current_sprite;
  Layer *layer = NULL;
  Image *image;
  Frame *frame;
  int index;

  if (sprite && name) {
    if (w == 0) w = sprite->w;
    if (h == 0) h = sprite->h;
 
    /* new image */
    image = image_new(sprite->imgtype, w, h);
    if (!image)
      return NULL;

    /* new layer */
    layer = layer_new(sprite->imgtype);
    if (!layer) {
      image_free(image);
      return NULL;
    }

    /* clear with mask color */
    image_clear(image, 0);

    /* configure layer name and blend mode */
    layer_set_name(layer, name);
    layer_set_blend_mode(layer, BLEND_MODE_NORMAL);

    /* add image in the layer stock */
    index = stock_add_image(layer->stock, image);

    /* create a new frame in the current frpos */
    frame = frame_new(sprite->frpos, index);
    frame_set_position(frame, x, y);

    /* add frame */
    layer_add_frame(layer, frame);

    /* undo stuff */
    if (undo_is_enabled(sprite->undo)) {
      undo_open(sprite->undo);
      undo_add_layer(sprite->undo, sprite->set, layer);
      undo_set_layer(sprite->undo, sprite);
      undo_close(sprite->undo);
    }

    /* add the layer in the sprite set */
    layer_add_layer(sprite->set, layer);

    /* select the new layer */
    sprite_set_layer(sprite, layer);
  }

  return layer;
}

/**
 * Creates a new layer set with the "name" in the current sprite
 */
Layer *NewLayerSet(const char *name)
{
  Sprite *sprite = current_sprite;
  Layer *layer = NULL;

  if (sprite && name) {
    /* new layer */
    layer = layer_set_new();
    if (!layer)
      return NULL;

    /* configure layer name and blend mode */
    layer_set_name(layer, name);

    /* add the layer in the sprite set */
    layer_add_layer(sprite->set, layer);

    /* select the new layer */
    sprite_set_layer(sprite, layer);
  }

  return layer;
}

/**
 * Removes the current selected layer
 */
void RemoveLayer(void)
{
  Sprite *sprite = current_sprite;
  if (sprite && sprite->layer) {
    Layer *layer = sprite->layer;
    Layer *parent = (Layer *)layer->parent;
    Layer *layer_select;

    /* select: previous layer, or next layer, or parent(if it is not
       the main layer of sprite set) */
    if (layer_get_prev(layer))
      layer_select = layer_get_prev(layer);
    else if (layer_get_next(layer))
      layer_select = layer_get_next(layer);
    else if (parent != sprite->set)
      layer_select = parent;
    else
      layer_select = NULL;

    /* undo stuff */
    if (undo_is_enabled(sprite->undo)) {
      undo_open(sprite->undo);
      undo_set_layer(sprite->undo, sprite);
      undo_remove_layer(sprite->undo, layer);
      undo_close(sprite->undo);
    }

    /* select other layer */
    sprite_set_layer(sprite, layer_select);

    /* remove the layer */
    layer_remove_layer(parent, layer);

    /* destroy the layer */
    layer_free(layer);
  }
}

Layer *FlattenLayers(void)
{
  JLink link, next;
  Layer *flat_layer;
  Sprite *sprite;

  sprite = current_sprite;
  if (!sprite)
    return NULL;

  /* generate the flat_layer */
  flat_layer = layer_flatten(sprite->set,
			     sprite->imgtype, 0, 0, sprite->w, sprite->h,
			     0, sprite->frames-1);
  if (!flat_layer) {
    console_printf("Not enough memory");
    return NULL;
  }

  /* open undo, and add the new layer */
  if (undo_is_enabled(sprite->undo)) {
    undo_open(sprite->undo);
    undo_add_layer(sprite->undo, sprite->set, flat_layer);
  }

  layer_add_layer(sprite->set, flat_layer);

  /* select the new layer */
  if (undo_is_enabled(sprite->undo))
    undo_set_layer(sprite->undo, sprite);

  sprite_set_layer(sprite, flat_layer);

  /* remove old layers */

  JI_LIST_FOR_EACH_SAFE(sprite->set->layers, link, next) {
    if (link->data != flat_layer) {
      /* undo */
      if (undo_is_enabled(sprite->undo))
	undo_remove_layer(sprite->undo, link->data);

      /* remove and destroy this layer */
      layer_remove_layer(sprite->set, link->data);
      layer_free(link->data);
    }
  }

  /* close the undo */
  if (undo_is_enabled(sprite->undo))
    undo_close(sprite->undo);

  GUI_Refresh(sprite);

  return flat_layer;
}

/* ======================================= */
/* Frame                                   */
/* ======================================= */

void RemoveFrame(Layer *layer, Frame *frame)
{
  Sprite *sprite = current_sprite;
  Image *image;
  Frame *it;
  int frpos;
  bool used;

  if (sprite != NULL && layer_is_image(layer) && frame != NULL) {
    /* find if the image that use the frame to remove, is used by
       another frames */
    used = FALSE;
    for (frpos=0; frpos<sprite->frames; ++frpos) {
      it = layer_get_frame(layer, frpos);
      if (it != NULL && it != frame && it->image == frame->image) {
	used = TRUE;
	break;
      }
    }

    undo_open(sprite->undo);
    if (!used) {
      /* if the image is only used by this frame, we can remove the
	 image from the stock */
      image = stock_get_image(layer->stock, frame->image);
      undo_remove_image(sprite->undo, layer->stock, image);
      stock_remove_image(layer->stock, image);
      image_free(image);
    }
    undo_remove_frame(sprite->undo, layer, frame);
    undo_close(sprite->undo);

    /* remove the frame */
    layer_remove_frame(layer, frame);
    frame_free(frame);
  }
}
