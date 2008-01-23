/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2005, 2007, 2008  David A. Capello
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

#include <assert.h>
#include <string.h>

#include "jinete/jlist.h"
#include "jinete/jmutex.h"

#include "modules/palette.h"
#include "raster/raster.h"
#include "util/boundary.h"

#endif

typedef struct PAL
{
  int frame;
  PALETTE pal;
} PAL;

static int index_count;

static Layer *index2layer(Layer *layer, int index);
static int layer2index(const Layer *layer, const Layer *find_layer);

static Sprite *general_copy(const Sprite *sprite);

Sprite *sprite_new(int imgtype, int w, int h)
{
  Sprite *sprite = (Sprite *)gfxobj_new(GFXOBJ_SPRITE, sizeof(Sprite));
  PALETTE pal;
  int c;

  if (!sprite)
    return NULL;

  /* main properties */
  strcpy(sprite->filename, "Sprite");
  sprite->associated_to_file = FALSE;
  sprite->imgtype = imgtype;
  sprite->w = w;
  sprite->h = h;
  sprite->bgcolor = 0;
  sprite->frames = 1;
  sprite->frlens = jmalloc(sizeof(int)*sprite->frames);
  sprite->frame = 0;
/*   sprite->palette = jmalloc(sizeof(PALETTE)); */
  sprite->palettes = jlist_new();
  sprite->stock = stock_new(imgtype);
  sprite->set = layer_set_new(sprite);
  sprite->layer = NULL;
  sprite->path = NULL;
  sprite->mask = mask_new();
  sprite->undo = undo_new(sprite);
  sprite->repository.paths = jlist_new();
  sprite->repository.masks = jlist_new();

  /* boundary stuff */
  sprite->bound.nseg = 0;
  sprite->bound.seg = NULL;

  /* preferred edition options */
  sprite->preferred.scroll_x = 0;
  sprite->preferred.scroll_y = 0;
  sprite->preferred.zoom = 0;

  /* setup the layer set */
  layer_set_parent (sprite->set, (GfxObj *)sprite);

  /* generate palette */
  switch (imgtype) {

    /* for colored images */
    case IMAGE_RGB:
    case IMAGE_INDEXED:
      for (c=0; c<256; c++) {
	pal[c].r = default_palette[c].r;
	pal[c].g = default_palette[c].g;
	pal[c].b = default_palette[c].b;
      }
      break;

    /* for black and white images */
    case IMAGE_GRAYSCALE:
    case IMAGE_BITMAP:
      for (c=0; c<256; c++) {
	pal[c].r = c >> 2;
	pal[c].g = c >> 2;
	pal[c].b = c >> 2;
      }
      break;
  }

  sprite_set_palette(sprite, pal, sprite->frame);
  sprite_set_speed(sprite, 42);

  /* multiple access */
  sprite->locked = FALSE;
  sprite->mutex = jmutex_new();
  
  return sprite;
}

Sprite *sprite_new_copy(const Sprite *sprite)
{
  Sprite *sprite_copy;
  int selected_layer;

  sprite_copy = general_copy(sprite);
  if (!sprite_copy)
    return NULL;

  /* copy layers */
  if (sprite_copy->set) {
    layer_free(sprite_copy->set);
    sprite_copy->set = NULL;
  }

  if (sprite->set) {
    sprite_copy->set = layer_new_copy(sprite->set);
    if (!sprite_copy->set) {
      sprite_free(sprite_copy);
      return NULL;
    }

    /* setup the layer set */
    layer_set_parent(sprite_copy->set, (GfxObj *)sprite_copy);
  }

  /* selected layer */
  if (sprite->layer) { 
    selected_layer = sprite_layer2index(sprite, sprite->layer);
    sprite_copy->layer = sprite_index2layer(sprite_copy, selected_layer);
  }

  sprite_generate_mask_boundaries(sprite_copy);

  return sprite_copy;
}

Sprite *sprite_new_flatten_copy(const Sprite *sprite)
{
  Sprite *sprite_copy;

  sprite_copy = general_copy(sprite);
  if (!sprite_copy)
    return NULL;

  /* flatten layers */
  if (sprite->set) {
    Layer *flat_layer = layer_flatten(sprite->set,
				      0, 0, sprite->w, sprite->h,
				      0, sprite->frames-1);
    if (!flat_layer) {
      sprite_free(sprite_copy);
      return NULL;
    }

    /* add and select the new flat layer */
    layer_add_layer(sprite_copy->set, flat_layer);
    sprite_copy->layer = flat_layer;
  }

  return sprite_copy;
}

Sprite *sprite_new_with_layer(int imgtype, int w, int h)
{
  Sprite *sprite;
  Layer *layer;
  Cel *cel;
  Image *image;
  int index;

  sprite = sprite_new(imgtype, w, h);
  if (!sprite)
    return NULL;

  /* new image */
  image = image_new(imgtype, w, h);
  if (!image) {
    sprite_free(sprite);
    return NULL;
  }

  /* new layer */
  layer = layer_new(sprite);
  if (!layer) {
    image_free(image);
    sprite_free(sprite);
    return NULL;
  }

  /* clear with mask color */
  image_clear(image, 0);

  /* configure layer name and blend mode */
  layer_set_name(layer, "Background");
  layer_set_blend_mode(layer, BLEND_MODE_NORMAL);

  /* add image in the layer stock */
  index = stock_add_image(sprite->stock, image);

  /* create the cel */
  cel = cel_new(0, index);
  cel_set_position(cel, 0, 0);

  /* add the cel in the layer */
  layer_add_cel(layer, cel);

  /* add the layer in the sprite */
  layer_add_layer(sprite->set, layer);

  sprite_set_frames(sprite, 1);
  sprite_set_filename(sprite, "Sprite");
  sprite_set_layer(sprite, layer);

  return sprite;
}

/**
 * Destroys the sprite
 */
void sprite_free(Sprite *sprite)
{
  JLink link;

  assert(!sprite->locked);

  /* destroy images' stock */
  if (sprite->stock)
    stock_free(sprite->stock);

  /* destroy paths */
  if (sprite->repository.paths) {
    JI_LIST_FOR_EACH(sprite->repository.paths, link)
      path_free(link->data);

    jlist_free(sprite->repository.paths);
  }

  /* destroy masks */
  if (sprite->repository.masks) {
    JI_LIST_FOR_EACH(sprite->repository.masks, link)
      mask_free(link->data);

    jlist_free(sprite->repository.masks);
  }

  /* destroy palettes */
  if (sprite->palettes) {
    JI_LIST_FOR_EACH(sprite->palettes, link)
      jfree(link->data);

    jlist_free(sprite->palettes);
  }

  /* destroy undo, mask, all layers, stock, boundaries */
  if (sprite->frlens) jfree(sprite->frlens);
  if (sprite->undo) undo_free(sprite->undo);
  if (sprite->mask) mask_free(sprite->mask);
  if (sprite->set) layer_free(sprite->set);
  if (sprite->bound.seg) jfree(sprite->bound.seg);

  /* destroy mutex */
  jmutex_free(sprite->mutex);

  gfxobj_free((GfxObj *)sprite);
}

bool sprite_is_modified(Sprite *sprite)
{
  return (sprite->undo->diff_count ==
	  sprite->undo->diff_saved) ? FALSE: TRUE;
}

bool sprite_is_associated_to_file(Sprite *sprite)
{
  return sprite->associated_to_file;
}

bool sprite_is_locked(Sprite *sprite)
{
  bool locked;

  jmutex_lock(sprite->mutex);
  locked = sprite->locked;
  jmutex_unlock(sprite->mutex);
    
  return locked;
}

void sprite_mark_as_saved(Sprite *sprite)
{
  sprite->undo->diff_saved = sprite->undo->diff_count;
  sprite->associated_to_file = TRUE;
}

bool sprite_lock(Sprite *sprite)
{
  bool res = FALSE;

  jmutex_lock(sprite->mutex);
  if (!sprite->locked) {
    sprite->locked = TRUE;
    res = TRUE;
  }
  jmutex_unlock(sprite->mutex);

  return res;
}

void sprite_unlock(Sprite *sprite)
{
  jmutex_lock(sprite->mutex);
  assert(sprite->locked);
  sprite->locked = FALSE;
  jmutex_unlock(sprite->mutex);
}

RGB *sprite_get_palette(Sprite *sprite, int frame)
{
  RGB *rgb = NULL;
  JLink link;
  PAL *pal;

  frame = MAX(0, frame);

  JI_LIST_FOR_EACH(sprite->palettes, link) {
    pal = link->data;
    if (frame < pal->frame)
      break;
    rgb = pal->pal;
    if (frame == pal->frame)
      break;
  }

  return rgb;
}

void sprite_set_palette(Sprite *sprite, RGB *rgb, int frame)
{
  JLink link;
  PAL *pal;

  frame = MAX(0, frame);

  JI_LIST_FOR_EACH(sprite->palettes, link) {
    pal = link->data;

    if (frame == pal->frame) {
      palette_copy(pal->pal, rgb);
      return;
    }
    else if (frame < pal->frame)
      break;
  }

  pal = jmalloc(sizeof(PAL));
  pal->frame = frame;
  palette_copy(pal->pal, rgb);

  jlist_insert_before(sprite->palettes, link, pal);
}

/**
 * Leaves the first palette only in the sprite
 */
void sprite_reset_palettes(Sprite *sprite)
{
  JLink link, next;

  JI_LIST_FOR_EACH_SAFE(sprite->palettes, link, next) {
    if (jlist_first(sprite->palettes) != link) {
      jfree(link->data);
      jlist_delete_link(sprite->palettes, link);
    }
  }
}

/**
 * Changes the sprite's filename
 */
void sprite_set_filename(Sprite *sprite, const char *filename)
{
  strcpy(sprite->filename, filename);
}

void sprite_set_size(Sprite *sprite, int w, int h)
{
  sprite->w = w;
  sprite->h = h;
}

/**
 * Changes the quantity of frames
 */
void sprite_set_frames(Sprite *sprite, int frames)
{
  frames = MAX(1, frames);

  sprite->frlens = jrealloc(sprite->frlens, sizeof(int)*frames);

  if (frames > sprite->frames) {
    int c;
    for (c=sprite->frames; c<frames; c++)
      sprite->frlens[c] = sprite->frlens[sprite->frames-1];
  }

  sprite->frames = frames;
}

void sprite_set_frlen(Sprite *sprite, int msecs, int frame)
{
  if (frame >= 0 && frame < sprite->frames)
    sprite->frlens[frame] = MID(1, msecs, 65535);
}

int sprite_get_frlen(Sprite *sprite, int frame)
{
  if (frame >= 0 && frame < sprite->frames)
    return sprite->frlens[frame];
  else
    return 0;
}

/**
 * Sets a constant frame-rate
 */
void sprite_set_speed(Sprite *sprite, int msecs)
{
  int c;
  for (c=0; c<sprite->frames; c++)
    sprite->frlens[c] = MID(1, msecs, 65535);
}

/**
 * Changes the current path (makes a copy of "path")
 */
void sprite_set_path(Sprite *sprite, const Path *path)
{
  if (sprite->path)
    path_free(sprite->path);

  sprite->path = path_new_copy(path);
}

/**
 * Changes the current mask (makes a copy of "mask")
 */
void sprite_set_mask(Sprite *sprite, const Mask *mask)
{
  if (sprite->mask)
    mask_free(sprite->mask);

  sprite->mask = mask_new_copy(mask);
}

/**
 * Changes the current layer
 */
void sprite_set_layer(Sprite *sprite, Layer *layer)
{
  sprite->layer = layer;
}

void sprite_set_frame(Sprite *sprite, int frame)
{
  sprite->frame = frame;
}

/**
 * @warning: it uses the current Allegro "rgb_map"
 */
void sprite_set_imgtype(Sprite *sprite, int imgtype, int dithering_method)
{
  RGB *palette = sprite_get_palette(sprite, 0);
  Image *old_image;
  Image *new_image;
  int c, r, g, b;

  /* nothing to do */
  if (sprite->imgtype == imgtype)
    return;

  /* if the undo is enabled, open a "big" group */
  if (undo_is_enabled(sprite->undo))
    undo_open(sprite->undo);

  /* change the background color */
  if (undo_is_enabled(sprite->undo))
    undo_int(sprite->undo, (GfxObj *)sprite, &sprite->bgcolor);

  c = sprite->bgcolor;

  switch (sprite->imgtype) {

    case IMAGE_RGB:
      switch (imgtype) {
	/* RGB -> Grayscale */
	case IMAGE_GRAYSCALE:
	  r = _rgba_getr(c);
	  g = _rgba_getg(c);
	  b = _rgba_getb(c);
	  rgb_to_hsv_int(&r, &g, &b);
	  c = _graya(b, _rgba_geta(c));
	  break;
	/* RGB -> Indexed */
	case IMAGE_INDEXED:
	  r = _rgba_getr(c);
	  g = _rgba_getg(c);
	  b = _rgba_getb(c);
	  if (_rgba_geta(c) == 0)
	    c = 0;
	  else
	    c = rgb_map->data[r>>3][g>>3][b>>3];
	  break;
      }
      break;

    case IMAGE_GRAYSCALE:
      switch (imgtype) {
	/* Grayscale -> RGB */
	case IMAGE_RGB:
	  g = _graya_getk(c);
	  c = _rgba(g, g, g, _graya_geta(c));
	  break;
	/* Grayscale -> Indexed */
	case IMAGE_INDEXED:
	  if (_graya_geta(c) == 0)
	    c = 0;
	  else
	    c = _graya_getk(c);
	  break;
      }
      break;

    case IMAGE_INDEXED:
      switch (imgtype) {
	/* Indexed -> RGB */
	case IMAGE_RGB:
	  if (c == 0)
	    c = 0;
	  else
	    c = _rgba(_rgb_scale_6[palette[c].r],
		      _rgb_scale_6[palette[c].g],
		      _rgb_scale_6[palette[c].b], 255);
	  break;
	/* Indexed -> Grayscale */
	case IMAGE_GRAYSCALE:
	  if (c == 0)
	    c = 0;
	  else {
	    r = _rgb_scale_6[palette[c].r];
	    g = _rgb_scale_6[palette[c].g];
	    b = _rgb_scale_6[palette[c].b];
	    rgb_to_hsv_int(&r, &g, &b);
	    c = _graya(b, 255);
	  }
	  break;
      }
      break;
  }

  sprite_set_bgcolor(sprite, c);
  
  /* change imgtype of the stock of images */
  if (undo_is_enabled(sprite->undo)) {
    undo_int(sprite->undo, (GfxObj *)sprite, &imgtype);
    undo_int(sprite->undo, (GfxObj *)sprite->stock, &sprite->stock->imgtype);
  }
  sprite->stock->imgtype = imgtype;

  for (c=0; c<sprite->stock->nimage; c++) {
    old_image = stock_get_image(sprite->stock, c);
    if (!old_image)
      continue;

    new_image = image_set_imgtype(old_image, imgtype, dithering_method,
				  rgb_map,
				  /* TODO check this out */
				  sprite_get_palette(sprite,
						     sprite->frame));
    if (!new_image)
      return;		/* TODO big error!!!: not enough memory!
			   we should undo all work done */

    if (undo_is_enabled(sprite->undo))
      undo_replace_image(sprite->undo, sprite->stock, c);

    image_free(old_image);
    stock_replace_image(sprite->stock, c, new_image);
  }

  /* change "sprite.imgtype" field */
  if (undo_is_enabled(sprite->undo))
    undo_int(sprite->undo, (GfxObj *)sprite, &sprite->imgtype);

  sprite->imgtype = imgtype;

#if 0				/* TODO */
  /* change "sprite.palette" */
  if (imgtype == IMAGE_GRAYSCALE) {
    int c;
    undo_data(sprite->undo, (GfxObj *)sprite,
	      &sprite->palette, sizeof(PALETTE));
    for (c=0; c<256; c++) {
      sprite->palette[c].r = c >> 2;
      sprite->palette[c].g = c >> 2;
      sprite->palette[c].b = c >> 2;
    }
  }
#endif

  if (undo_is_enabled(sprite->undo))
    undo_close(sprite->undo);
}

/**
 * Sets the background color of the sprite.
 */
void sprite_set_bgcolor(Sprite *sprite, int color)
{
  sprite->bgcolor = color;
}

/**
 * Adds a path to the sprites's repository
 */
void sprite_add_path(Sprite *sprite, Path *path)
{
  jlist_append(sprite->repository.paths, path);
}

/**
 * Removes a path from the sprites's repository
 */
void sprite_remove_path (Sprite *sprite, Path *path)
{
  jlist_remove(sprite->repository.paths, path);
}

/**
 * Adds a mask to the sprites's repository
 */
void sprite_add_mask (Sprite *sprite, Mask *mask)
{
  /* you can't add masks to repository without name */
  if (!mask->name)
    return;

  /* you can't add a mask that already exists */
  if (sprite_request_mask(sprite, mask->name))
    return;

  /* and add the new mask */
  jlist_append(sprite->repository.masks, mask);
}

/**
 * Removes a mask from the sprites's repository
 */
void sprite_remove_mask(Sprite *sprite, Mask *mask)
{
  /* remove the mask from the repository */
  jlist_remove(sprite->repository.masks, mask);
}

/**
 * Returns a mask from the sprite's repository searching it by its name
 */
Mask *sprite_request_mask(Sprite *sprite, const char *name)
{
  Mask *mask;
  JLink link;

  JI_LIST_FOR_EACH(sprite->repository.masks, link) {
    mask = link->data;
    if (strcmp(mask->name, name) == 0)
      return mask;
  }

  return NULL;
}

void sprite_render(Sprite *sprite, Image *image, int x, int y)
{
  image_rectfill(image, x, y, x+sprite->w-1, y+sprite->h-1, sprite->bgcolor);
  layer_render(sprite->set, image, x, y, sprite->frame);
}

void sprite_generate_mask_boundaries(Sprite *sprite)
{
  int c;

  if (sprite->bound.seg) {
    jfree(sprite->bound.seg);
    sprite->bound.seg = NULL;
    sprite->bound.nseg = 0;
  }

  if (sprite->mask->bitmap) {
    sprite->bound.seg = find_mask_boundary(sprite->mask->bitmap,
					   &sprite->bound.nseg,
					   IgnoreBounds, 0, 0, 0, 0);
    for (c=0; c<sprite->bound.nseg; c++) {
      sprite->bound.seg[c].x1 += sprite->mask->x;
      sprite->bound.seg[c].y1 += sprite->mask->y;
      sprite->bound.seg[c].x2 += sprite->mask->x;
      sprite->bound.seg[c].y2 += sprite->mask->y;
    }
  }
}

Layer *sprite_index2layer(Sprite *sprite, int index)
{
  index_count = -1;

  return index2layer(sprite->set, index);
}

int sprite_layer2index(const Sprite *sprite, const Layer *layer)
{
  index_count = -1;

  return layer2index(sprite->set, layer);
}

static Layer *index2layer(Layer *layer, int index)
{
  if (index == index_count)
    return layer;
  else {
    index_count++;

    if (layer_is_set (layer)) {
      Layer *found;
      JLink link;

      JI_LIST_FOR_EACH(layer->layers, link) {
	if ((found = index2layer(link->data, index)))
	  return found;
      }
    }

    return NULL;
  }
}

static int layer2index(const Layer *layer, const Layer *find_layer)
{
  if (layer == find_layer)
    return index_count;
  else {
    index_count++;

    if (layer_is_set (layer)) {
      JLink link;
      int found;

      JI_LIST_FOR_EACH(layer->layers, link) {
	if ((found = layer2index(link->data, find_layer)) >= 0)
	  return found;
      }
    }

    return -1;
  }
}

/**
 * Makes a copy "sprite" without the layers (only with the empty layer set)
 */
static Sprite *general_copy(const Sprite *sprite)
{
  Sprite *sprite_copy;
  JLink link;

  sprite_copy = sprite_new(sprite->imgtype, sprite->w, sprite->h);
  if (!sprite_copy)
    return NULL;

  /* copy stock */
  stock_free(sprite_copy->stock);
  sprite_copy->stock = stock_new_copy(sprite->stock);
  if (!sprite_copy->stock) {
    sprite_free(sprite_copy);
    return NULL;
  }

  /* copy general properties */
  strcpy(sprite_copy->filename, sprite->filename);
  sprite_set_bgcolor(sprite_copy, sprite->bgcolor);
  sprite_set_frames(sprite_copy, sprite->frame);
  memcpy(sprite_copy->frlens, sprite->frlens, sizeof(int)*sprite->frames);

  /* copy color palettes */
  JI_LIST_FOR_EACH(sprite->palettes, link) {
    PAL *pal = link->data;
    sprite_set_palette(sprite_copy, pal->pal, pal->frame);
  }

  /* copy path */
  if (sprite_copy->path) {
    path_free(sprite_copy->path);
    sprite_copy->path = NULL;
  }

  if (sprite->path) {
    sprite_copy->path = path_new_copy(sprite->path);
    if (!sprite_copy->path) {
      sprite_free(sprite_copy);
      return NULL;
    }
  }

  /* copy mask */
  if (sprite_copy->mask) {
    mask_free(sprite_copy->mask);
    sprite_copy->mask = NULL;
  }

  if (sprite->mask) {
    sprite_copy->mask = mask_new_copy(sprite->mask);
    if (!sprite_copy->mask) {
      sprite_free(sprite_copy);
      return NULL;
    }
  }

  /* copy repositories */
  JI_LIST_FOR_EACH(sprite->repository.paths, link) {
    Path *path_copy = path_new_copy(link->data);
    if (path_copy)
      sprite_add_path(sprite_copy, path_copy);
  }

  JI_LIST_FOR_EACH(sprite->repository.masks, link) {
    Mask *mask_copy = mask_new_copy(link->data);
    if (mask_copy)
      sprite_add_mask(sprite_copy, mask_copy);
  }

  /* copy preferred edition options */
  sprite_copy->preferred.scroll_x = sprite->preferred.scroll_x;
  sprite_copy->preferred.scroll_y = sprite->preferred.scroll_y;
  sprite_copy->preferred.zoom = sprite->preferred.zoom;

  return sprite_copy;
}
