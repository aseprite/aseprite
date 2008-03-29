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
#include <string.h>

#include "jinete/jlist.h"
#include "jinete/jmutex.h"

#include "file/format_options.h"
#include "modules/palettes.h"
#include "raster/raster.h"
#include "util/boundary.h"

static Layer *index2layer(Layer *layer, int index, int *index_count);
static int layer2index(const Layer *layer, const Layer *find_layer, int *index_count);

static Sprite *general_copy(const Sprite *src_sprite);

Sprite *sprite_new(int imgtype, int w, int h)
{
  Sprite *sprite;
  Palette *pal;
  int c;

  assert(w > 0 && h > 0);

  sprite = (Sprite *)gfxobj_new(GFXOBJ_SPRITE, sizeof(Sprite));
  if (!sprite)
    return NULL;

  /* main properties */
  strcpy(sprite->filename, "Sprite");
  sprite->associated_to_file = FALSE;
  sprite->imgtype = imgtype;
  sprite->w = w;
  sprite->h = h;
  sprite->frames = 1;
  sprite->frlens = jmalloc(sizeof(int)*sprite->frames);
  sprite->frame = 0;
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

  /* generate palette */
  pal = palette_new(0, MAX_PALETTE_COLORS);
  switch (imgtype) {

    /* for colored images */
    case IMAGE_RGB:
    case IMAGE_INDEXED:
      palette_copy_colors(pal, get_default_palette());
      break;

    /* for black and white images */
    case IMAGE_GRAYSCALE:
    case IMAGE_BITMAP:
      for (c=0; c<256; c++)
	palette_set_entry(pal, c, _rgba(c, c, c, 255));
      break;
  }
  sprite_set_palette(sprite, pal, TRUE);
  sprite_set_speed(sprite, 100);

  /* multiple access */
  sprite->locked = FALSE;
  sprite->mutex = jmutex_new();

  /* file format options */
  sprite->format_options = NULL;

  /* free the temporary palette */
  palette_free(pal);
  
  return sprite;
}

Sprite *sprite_new_copy(const Sprite *src_sprite)
{
  Sprite *dst_sprite;
  int selected_layer;

  assert(src_sprite != NULL);

  dst_sprite = general_copy(src_sprite);
  if (!dst_sprite)
    return NULL;

  /* copy layers */
  if (dst_sprite->set) {
    layer_free(dst_sprite->set);
    dst_sprite->set = NULL;
  }

  assert(src_sprite->set != NULL);

  undo_disable(dst_sprite->undo);
  dst_sprite->set = layer_new_copy(dst_sprite, src_sprite->set);
  undo_enable(dst_sprite->undo);

  if (dst_sprite->set == NULL) {
    sprite_free(dst_sprite);
    return NULL;
  }

  /* selected layer */
  if (src_sprite->layer != NULL) { 
    selected_layer = sprite_layer2index(src_sprite, src_sprite->layer);
    dst_sprite->layer = sprite_index2layer(dst_sprite, selected_layer);
  }

  sprite_generate_mask_boundaries(dst_sprite);

  return dst_sprite;
}

Sprite *sprite_new_flatten_copy(const Sprite *src_sprite)
{
  Sprite *dst_sprite;
  Layer *flat_layer;

  assert(src_sprite != NULL);

  dst_sprite = general_copy(src_sprite);
  if (dst_sprite == NULL)
    return NULL;

  /* flatten layers */
  assert(src_sprite->set != NULL);

  flat_layer = layer_new_flatten_copy(dst_sprite,
				      src_sprite->set,
				      0, 0, src_sprite->w, src_sprite->h,
				      0, src_sprite->frames-1);
  if (flat_layer == NULL) {
    sprite_free(dst_sprite);
    return NULL;
  }

  /* add and select the new flat layer */
  layer_add_layer(dst_sprite->set, flat_layer);
  dst_sprite->layer = flat_layer;

  return dst_sprite;
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

  /* configure the first transparent layer */
  layer_set_name(layer, "Layer 1");
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

  assert(sprite != NULL);
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

  /* destroy file format options */
  if (sprite->format_options)
    format_options_free(sprite->format_options);

  /* destroy gfxobj */
  gfxobj_free((GfxObj *)sprite);
}

bool sprite_is_modified(Sprite *sprite)
{
  assert(sprite != NULL);

  return (sprite->undo->diff_count ==
	  sprite->undo->diff_saved) ? FALSE: TRUE;
}

bool sprite_is_associated_to_file(Sprite *sprite)
{
  assert(sprite != NULL);

  return sprite->associated_to_file;
}

bool sprite_is_locked(Sprite *sprite)
{
  bool locked;

  assert(sprite != NULL);

  jmutex_lock(sprite->mutex);
  locked = sprite->locked;
  jmutex_unlock(sprite->mutex);
    
  return locked;
}

void sprite_mark_as_saved(Sprite *sprite)
{
  assert(sprite != NULL);

  sprite->undo->diff_saved = sprite->undo->diff_count;
  sprite->associated_to_file = TRUE;
}

bool sprite_need_alpha(Sprite *sprite)
{
  assert(sprite != NULL);

  switch (sprite->imgtype) {

    case IMAGE_RGB:
    case IMAGE_GRAYSCALE:
      return sprite_get_background_layer(sprite) == NULL;

  }
  return FALSE;
}

bool sprite_lock(Sprite *sprite)
{
  bool res = FALSE;

  assert(sprite != NULL);

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
  assert(sprite != NULL);

  jmutex_lock(sprite->mutex);
  assert(sprite->locked);
  sprite->locked = FALSE;
  jmutex_unlock(sprite->mutex);
}

Palette *sprite_get_palette(Sprite *sprite, int frame)
{
  Palette *found = NULL;
  Palette *pal;
  JLink link;

  assert(sprite != NULL);
  assert(frame >= 0);

  JI_LIST_FOR_EACH(sprite->palettes, link) {
    pal = link->data;
    if (frame < pal->frame)
      break;

    found = pal;
    if (frame == pal->frame)
      break;
  }

  assert(found != NULL);
  return found;
}

void sprite_set_palette(Sprite *sprite, Palette *pal, bool truncate)
{
  assert(sprite != NULL);

  if (!truncate) {
    Palette *sprite_pal = sprite_get_palette(sprite, sprite->frame);
    palette_copy_colors(sprite_pal, pal);
  }
  else {
    JLink link;
    Palette *other;

    JI_LIST_FOR_EACH(sprite->palettes, link) {
      other = link->data;

      if (pal->frame == other->frame) {
	palette_copy_colors(other, pal);
	return;
      }
      else if (pal->frame < other->frame)
	break;
    }

    jlist_insert_before(sprite->palettes, link,
			palette_new_copy(pal));
  }
}

/**
 * Leaves the first palette only in the sprite
 */
void sprite_reset_palettes(Sprite *sprite)
{
  JLink link, next;

  JI_LIST_FOR_EACH_SAFE(sprite->palettes, link, next) {
    if (jlist_first(sprite->palettes) != link) {
      palette_free(link->data);
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

void sprite_set_format_options(Sprite *sprite, struct FormatOptions *format_options)
{
  if (sprite->format_options)
    jfree(sprite->format_options);

  sprite->format_options = format_options;
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
  Image *old_image;
  Image *new_image;
  int c;

  /* nothing to do */
  if (sprite->imgtype == imgtype)
    return;

  /* if the undo is enabled, open a "big" group */
  if (undo_is_enabled(sprite->undo)) {
    undo_set_label(sprite->undo, "Color Mode Change");
    undo_open(sprite->undo);
  }

  /* change imgtype of the stock of images */
  if (undo_is_enabled(sprite->undo))
    undo_int(sprite->undo, (GfxObj *)sprite->stock, &sprite->stock->imgtype);

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
      return;		/* TODO error handling: not enough memory!
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

Layer *sprite_get_background_layer(Sprite *sprite)
{
  assert(sprite != NULL);

  if (jlist_length(sprite->set->layers) > 0) {
    Layer *bglayer = jlist_first_data(sprite->set->layers);

    if (layer_is_background(bglayer))
      return bglayer;
  }

  return NULL;
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
  image_rectfill(image, x, y, x+sprite->w-1, y+sprite->h-1, 0);
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
  int index_count = -1;

  return index2layer(sprite->set, index, &index_count);
}

int sprite_layer2index(const Sprite *sprite, const Layer *layer)
{
  int index_count = -1;

  return layer2index(sprite->set, layer, &index_count);
}

/**
 * Gets a pixel from the sprite in the specified position. If in the
 * specified coordinates there're background this routine will return
 * the 0 color (the mask-color).
 */
int sprite_getpixel(Sprite *sprite, int x, int y)
{
  Image *image;
  int color = 0;

  if ((x >= 0) && (y >= 0) && (x < sprite->w) && (y < sprite->h)) {
    image = image_new(sprite->imgtype, 1, 1);
    image_clear(image, 0);
    sprite_render(sprite, image, -x, -y);
    color = image_getpixel(image, 0, 0);
    image_free(image);
  }

  return color;
}

int sprite_get_memsize(Sprite *sprite)
{
  Image *image;
  int i, size = 0;

  for (i=0; i<sprite->stock->nimage; i++) {
    image = sprite->stock->image[i];

    if (image != NULL)
      size += IMAGE_LINE_SIZE(image, image->w) * image->h;
  }

  return size;
}

static Layer *index2layer(Layer *layer, int index, int *index_count)
{
  if (index == *index_count)
    return layer;
  else {
    (*index_count)++;

    if (layer_is_set (layer)) {
      Layer *found;
      JLink link;

      JI_LIST_FOR_EACH(layer->layers, link) {
	if ((found = index2layer(link->data, index, index_count)))
	  return found;
      }
    }

    return NULL;
  }
}

static int layer2index(const Layer *layer, const Layer *find_layer, int *index_count)
{
  if (layer == find_layer)
    return *index_count;
  else {
    (*index_count)++;

    if (layer_is_set(layer)) {
      JLink link;
      int found;

      JI_LIST_FOR_EACH(layer->layers, link) {
	if ((found = layer2index(link->data, find_layer, index_count)) >= 0)
	  return found;
      }
    }

    return -1;
  }
}

/**
 * Makes a copy "sprite" without the layers (only with the empty layer set)
 */
static Sprite *general_copy(const Sprite *src_sprite)
{
  Sprite *dst_sprite;
  JLink link;

  dst_sprite = sprite_new(src_sprite->imgtype, src_sprite->w, src_sprite->h);
  if (!dst_sprite)
    return NULL;

  /* copy stock */
  stock_free(dst_sprite->stock);
  dst_sprite->stock = stock_new_copy(src_sprite->stock);
  if (!dst_sprite->stock) {
    sprite_free(dst_sprite);
    return NULL;
  }

  /* copy general properties */
  strcpy(dst_sprite->filename, src_sprite->filename);
  sprite_set_frames(dst_sprite, src_sprite->frames);
  memcpy(dst_sprite->frlens, src_sprite->frlens, sizeof(int)*src_sprite->frames);

  /* copy color palettes */
  JI_LIST_FOR_EACH(src_sprite->palettes, link) {
    Palette *pal = link->data;
    sprite_set_palette(dst_sprite, pal, TRUE);
  }

  /* copy path */
  if (dst_sprite->path) {
    path_free(dst_sprite->path);
    dst_sprite->path = NULL;
  }

  if (src_sprite->path) {
    dst_sprite->path = path_new_copy(src_sprite->path);
    if (!dst_sprite->path) {
      sprite_free(dst_sprite);
      return NULL;
    }
  }

  /* copy mask */
  if (dst_sprite->mask) {
    mask_free(dst_sprite->mask);
    dst_sprite->mask = NULL;
  }

  if (src_sprite->mask) {
    dst_sprite->mask = mask_new_copy(src_sprite->mask);
    if (!dst_sprite->mask) {
      sprite_free(dst_sprite);
      return NULL;
    }
  }

  /* copy repositories */
  JI_LIST_FOR_EACH(src_sprite->repository.paths, link) {
    Path *path_copy = path_new_copy(link->data);
    if (path_copy)
      sprite_add_path(dst_sprite, path_copy);
  }

  JI_LIST_FOR_EACH(src_sprite->repository.masks, link) {
    Mask *mask_copy = mask_new_copy(link->data);
    if (mask_copy)
      sprite_add_mask(dst_sprite, mask_copy);
  }

  /* copy preferred edition options */
  dst_sprite->preferred.scroll_x = src_sprite->preferred.scroll_x;
  dst_sprite->preferred.scroll_y = src_sprite->preferred.scroll_y;
  dst_sprite->preferred.zoom = src_sprite->preferred.zoom;

  return dst_sprite;
}
