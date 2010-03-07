/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

static Layer *index2layer(const Layer *layer, int index, int *index_count);
static int layer2index(const Layer *layer, const Layer *find_layer, int *index_count);
static int layer_count_layers(const Layer *layer);

static Sprite* general_copy(const Sprite* src_sprite);

//////////////////////////////////////////////////////////////////////

Sprite::Sprite(int imgtype, int w, int h)
  : GfxObj(GFXOBJ_SPRITE)
{
  assert(w > 0 && h > 0);

  /* main properties */
  strcpy(this->filename, "Sprite");
  this->associated_to_file = false;
  this->imgtype = imgtype;
  this->w = w;
  this->h = h;
  this->frames = 1;
  this->frlens = (int*)jmalloc(sizeof(int)*this->frames);
  this->frame = 0;
  this->palettes = jlist_new();
  this->stock = stock_new(imgtype);
  this->m_folder = new LayerFolder(this);
  this->layer = NULL;
  this->path = NULL;
  this->mask = mask_new();
  this->undo = undo_new(this);
  this->repository.paths = jlist_new();
  this->repository.masks = jlist_new();
  this->m_extras = NULL;

  /* boundary stuff */
  this->bound.nseg = 0;
  this->bound.seg = NULL;

  /* preferred edition options */
  this->preferred.scroll_x = 0;
  this->preferred.scroll_y = 0;
  this->preferred.zoom = 0;

  /* generate palette */
  Palette* pal = palette_new(0, MAX_PALETTE_COLORS);
  switch (imgtype) {

    /* for colored images */
    case IMAGE_RGB:
    case IMAGE_INDEXED:
      palette_copy_colors(pal, get_default_palette());
      break;

    /* for black and white images */
    case IMAGE_GRAYSCALE:
    case IMAGE_BITMAP:
      for (int c=0; c<256; c++)
	palette_set_entry(pal, c, _rgba(c, c, c, 255));
      break;
  }
  sprite_set_palette(this, pal, true);
  sprite_set_speed(this, 100);

  /* multiple access */
  m_write_lock = false;
  m_read_locks = 0;
  m_mutex = jmutex_new();

  /* file format options */
  this->format_options = NULL;

  /* free the temporary palette */
  palette_free(pal);
}

Sprite::~Sprite()
{
  JLink link;

  // destroy layers
  delete m_folder;		// destroy layers

  // destroy images' stock
  if (this->stock)
    stock_free(this->stock);

  /* destroy paths */
  if (this->repository.paths) {
    JI_LIST_FOR_EACH(this->repository.paths, link)
      path_free(reinterpret_cast<Path*>(link->data));

    jlist_free(this->repository.paths);
  }

  /* destroy masks */
  if (this->repository.masks) {
    JI_LIST_FOR_EACH(this->repository.masks, link)
      mask_free(reinterpret_cast<Mask*>(link->data));

    jlist_free(this->repository.masks);
  }

  /* destroy palettes */
  if (this->palettes) {
    JI_LIST_FOR_EACH(this->palettes, link)
      palette_free(reinterpret_cast<Palette*>(link->data));

    jlist_free(this->palettes);
  }

  // destroy undo, mask, etc.
  delete this->undo;
  delete this->mask;
  delete this->m_extras;	// image
  if (this->frlens) jfree(this->frlens);
  if (this->bound.seg) jfree(this->bound.seg);

  /* destroy mutex */
  jmutex_free(this->m_mutex);

  /* destroy file format options */
  if (this->format_options)
    format_options_free(this->format_options);
}

//////////////////////////////////////////////////////////////////////

Sprite* sprite_new(int imgtype, int w, int h)
{
  return new Sprite(imgtype, w, h);
}

Sprite* sprite_new_copy(const Sprite* src_sprite)
{
  assert(src_sprite != NULL);

  Sprite* dst_sprite = general_copy(src_sprite);
  if (!dst_sprite)
    return NULL;

  /* copy layers */
  if (dst_sprite->m_folder) {
    delete dst_sprite->m_folder; // delete
    dst_sprite->m_folder = NULL;
  }

  assert(src_sprite->get_folder() != NULL);

  undo_disable(dst_sprite->undo);
  dst_sprite->m_folder = src_sprite->get_folder()->duplicate_for(dst_sprite);
  undo_enable(dst_sprite->undo);

  if (dst_sprite->m_folder == NULL) {
    delete dst_sprite;
    return NULL;
  }

  /* selected layer */
  if (src_sprite->layer != NULL) { 
    int selected_layer = sprite_layer2index(src_sprite, src_sprite->layer);
    dst_sprite->layer = sprite_index2layer(dst_sprite, selected_layer);
  }

  sprite_generate_mask_boundaries(dst_sprite);
  return dst_sprite;
}

Sprite* sprite_new_flatten_copy(const Sprite* src_sprite)
{
  Sprite* dst_sprite;
  Layer *flat_layer;

  assert(src_sprite != NULL);

  dst_sprite = general_copy(src_sprite);
  if (dst_sprite == NULL)
    return NULL;

  /* flatten layers */
  assert(src_sprite->get_folder() != NULL);

  flat_layer = layer_new_flatten_copy(dst_sprite,
				      src_sprite->get_folder(),
				      0, 0, src_sprite->w, src_sprite->h,
				      0, src_sprite->frames-1);
  if (flat_layer == NULL) {
    delete dst_sprite;
    return NULL;
  }

  /* add and select the new flat layer */
  dst_sprite->get_folder()->add_layer(flat_layer);
  dst_sprite->layer = flat_layer;

  return dst_sprite;
}

Sprite* sprite_new_with_layer(int imgtype, int w, int h)
{
  Sprite* sprite = NULL;
  LayerImage *layer = NULL;
  Image *image = NULL;
  Cel *cel = NULL;

  try {
    sprite = sprite_new(imgtype, w, h);
    image = image_new(imgtype, w, h);
    layer = new LayerImage(sprite);

    /* clear with mask color */
    image_clear(image, 0);

    /* configure the first transparent layer */
    layer->set_name("Layer 1");
    layer->set_blend_mode(BLEND_MODE_NORMAL);

    /* add image in the layer stock */
    int index = stock_add_image(sprite->stock, image);

    /* create the cel */
    cel = cel_new(0, index);
    cel_set_position(cel, 0, 0);

    /* add the cel in the layer */
    layer->add_cel(cel);

    /* add the layer in the sprite */
    sprite->get_folder()->add_layer(layer);

    sprite_set_frames(sprite, 1);
    sprite_set_filename(sprite, "Sprite");
    sprite_set_layer(sprite, layer);
  }
  catch (...) {
    delete sprite;
    delete image;
    throw;
  }

  return sprite;
}

bool sprite_is_modified(const Sprite* sprite)
{
  assert(sprite != NULL);

  return (sprite->undo->diff_count ==
	  sprite->undo->diff_saved) ? false: true;
}

bool sprite_is_associated_to_file(const Sprite* sprite)
{
  assert(sprite != NULL);

  return sprite->associated_to_file;
}

void sprite_mark_as_saved(Sprite* sprite)
{
  assert(sprite != NULL);

  sprite->undo->diff_saved = sprite->undo->diff_count;
  sprite->associated_to_file = true;
}

bool sprite_need_alpha(const Sprite* sprite)
{
  assert(sprite != NULL);

  switch (sprite->imgtype) {

    case IMAGE_RGB:
    case IMAGE_GRAYSCALE:
      return sprite_get_background_layer(sprite) == NULL;

  }
  return false;
}

/**
 * Lock the sprite to read or write it.
 *
 * @return true if the sprite can be accessed in the desired mode.
 */
bool Sprite::lock(bool write)
{
  ScopedLock hold(m_mutex);

  // read-only
  if (!write) {
    // If no body is writting the sprite...
    if (!m_write_lock) {
      // We can read it
      ++m_read_locks;
      return true;
    }
  }
  // read and write
  else {
    // If no body is reading and writting...
    if (m_read_locks == 0 && !m_write_lock) {
      // We can start writting the sprite...
      m_write_lock = true;
      return true;
    }
  }

  return false;
}

/**
 * If you have locked the sprite to read, using this method
 * you can raise your access level to write it.
 */
bool Sprite::lock_to_write()
{
  ScopedLock hold(m_mutex);

  // this only is possible if there are just one reader
  if (m_read_locks == 1) {
    assert(!m_write_lock);
    m_read_locks = 0;
    m_write_lock = true;
    return true;
  }
  else
    return false;
}

/**
 * If you have locked the sprite to write, using this method
 * you can your access level to only read it.
 */
void Sprite::unlock_to_read()
{
  ScopedLock hold(m_mutex);
  assert(m_read_locks == 0);
  assert(m_write_lock);

  m_write_lock = false;
  m_read_locks = 1;
}

void Sprite::unlock()
{
  ScopedLock hold(m_mutex);

  if (m_write_lock) {
    m_write_lock = false;
  }
  else if (m_read_locks > 0) {
    --m_read_locks;
  }
  else {
    assert(false);
  }
}

void Sprite::prepare_extra()
{
  if (!m_extras ||
      m_extras->imgtype != imgtype ||
      m_extras->w != w ||
      m_extras->h != h) {
    delete m_extras;		// image
    m_extras = image_new(imgtype, w, h);
    image_clear(m_extras, 0);
  }
}

Palette* sprite_get_palette(const Sprite* sprite, int frame)
{
  Palette* found = NULL;
  Palette* pal;
  JLink link;

  assert(sprite != NULL);
  assert(frame >= 0);

  JI_LIST_FOR_EACH(sprite->palettes, link) {
    pal = reinterpret_cast<Palette*>(link->data);
    if (frame < pal->frame)
      break;

    found = pal;
    if (frame == pal->frame)
      break;
  }

  assert(found != NULL);
  return found;
}

void sprite_set_palette(Sprite* sprite, Palette* pal, bool truncate)
{
  assert(sprite != NULL);
  assert(pal != NULL);

  if (!truncate) {
    Palette* sprite_pal = sprite_get_palette(sprite, pal->frame);
    palette_copy_colors(sprite_pal, pal);
  }
  else {
    JLink link;
    Palette* other;

    JI_LIST_FOR_EACH(sprite->palettes, link) {
      other = reinterpret_cast<Palette*>(link->data);

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
void sprite_reset_palettes(Sprite* sprite)
{
  JLink link, next;

  JI_LIST_FOR_EACH_SAFE(sprite->palettes, link, next) {
    if (jlist_first(sprite->palettes) != link) {
      palette_free(reinterpret_cast<Palette*>(link->data));
      jlist_delete_link(sprite->palettes, link);
    }
  }
}

void sprite_delete_palette(Sprite* sprite, Palette* pal)
{
  assert(sprite != NULL);
  assert(pal != NULL);

  JLink link = jlist_find(sprite->palettes, pal);
  assert(link != NULL);

  palette_free(pal);
  jlist_delete_link(sprite->palettes, link);
}

/**
 * Changes the sprite's filename
 */
void sprite_set_filename(Sprite* sprite, const char *filename)
{
  strcpy(sprite->filename, filename);
}

void sprite_set_format_options(Sprite* sprite, struct FormatOptions *format_options)
{
  if (sprite->format_options)
    jfree(sprite->format_options);

  sprite->format_options = format_options;
}

void sprite_set_size(Sprite* sprite, int w, int h)
{
  assert(w > 0);
  assert(h > 0);

  sprite->w = w;
  sprite->h = h;
}

/**
 * Changes the quantity of frames
 */
void sprite_set_frames(Sprite* sprite, int frames)
{
  frames = MAX(1, frames);

  sprite->frlens = (int*)jrealloc(sprite->frlens, sizeof(int)*frames);

  if (frames > sprite->frames) {
    int c;
    for (c=sprite->frames; c<frames; c++)
      sprite->frlens[c] = sprite->frlens[sprite->frames-1];
  }

  sprite->frames = frames;
}

void sprite_set_frlen(Sprite* sprite, int frame, int msecs)
{
  if (frame >= 0 && frame < sprite->frames)
    sprite->frlens[frame] = MID(1, msecs, 65535);
}

int sprite_get_frlen(const Sprite* sprite, int frame)
{
  if (frame >= 0 && frame < sprite->frames)
    return sprite->frlens[frame];
  else
    return 0;
}

/**
 * Sets a constant frame-rate.
 */
void sprite_set_speed(Sprite* sprite, int msecs)
{
  int c;
  for (c=0; c<sprite->frames; c++)
    sprite->frlens[c] = MID(1, msecs, 65535);
}

/**
 * Changes the current path (makes a copy of "path")
 */
void sprite_set_path(Sprite* sprite, const Path* path)
{
  if (sprite->path)
    path_free(sprite->path);

  sprite->path = path_new_copy(path);
}

/**
 * Changes the current mask (makes a copy of "mask")
 */
void sprite_set_mask(Sprite* sprite, const Mask *mask)
{
  if (sprite->mask)
    mask_free(sprite->mask);

  sprite->mask = mask_new_copy(mask);
}

/**
 * Changes the current layer
 */
void sprite_set_layer(Sprite* sprite, Layer *layer)
{
  sprite->layer = layer;
}

void sprite_set_frame(Sprite* sprite, int frame)
{
  sprite->frame = frame;
}

LayerImage* sprite_get_background_layer(const Sprite* sprite)
{
  assert(sprite != NULL);

  if (sprite->get_folder()->get_layers_count() > 0) {
    Layer* bglayer = *sprite->get_folder()->get_layer_begin();

    if (bglayer->is_background())
      return static_cast<LayerImage*>(bglayer);
  }

  return NULL;
}

/**
 * Adds a path to the sprites's repository
 */
void sprite_add_path(Sprite* sprite, Path* path)
{
  jlist_append(sprite->repository.paths, path);
}

/**
 * Removes a path from the sprites's repository
 */
void sprite_remove_path (Sprite* sprite, Path* path)
{
  jlist_remove(sprite->repository.paths, path);
}

/**
 * Adds a mask to the sprites's repository
 */
void sprite_add_mask (Sprite* sprite, Mask *mask)
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
void sprite_remove_mask(Sprite* sprite, Mask *mask)
{
  /* remove the mask from the repository */
  jlist_remove(sprite->repository.masks, mask);
}

/**
 * Returns a mask from the sprite's repository searching it by its name
 */
Mask *sprite_request_mask(const Sprite* sprite, const char *name)
{
  Mask *mask;
  JLink link;

  JI_LIST_FOR_EACH(sprite->repository.masks, link) {
    mask = reinterpret_cast<Mask*>(link->data);
    if (strcmp(mask->name, name) == 0)
      return mask;
  }

  return NULL;
}

void sprite_render(const Sprite* sprite, Image *image, int x, int y)
{
  image_rectfill(image, x, y, x+sprite->w-1, y+sprite->h-1, 0);
  layer_render(sprite->get_folder(), image, x, y, sprite->frame);
}

void sprite_generate_mask_boundaries(Sprite* sprite)
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

Layer* sprite_index2layer(const Sprite* sprite, int index)
{
  int index_count = -1;
  assert(sprite != NULL);

  return index2layer(sprite->get_folder(), index, &index_count);
}

int sprite_layer2index(const Sprite* sprite, const Layer *layer)
{
  int index_count = -1;
  assert(sprite != NULL);

  return layer2index(sprite->get_folder(), layer, &index_count);
}

int sprite_count_layers(const Sprite* sprite)
{
  assert(sprite != NULL);
  return sprite->get_folder()->get_layers_count();
}

void sprite_get_cels(const Sprite* sprite, CelList& cels)
{
  sprite->get_folder()->get_cels(cels);
}

/**
 * Gets a pixel from the sprite in the specified position. If in the
 * specified coordinates there're background this routine will return
 * the 0 color (the mask-color).
 */
int sprite_getpixel(const Sprite* sprite, int x, int y)
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

int sprite_get_memsize(const Sprite* sprite)
{
  Image *image;
  int i, size = 0;

  for (i=0; i<sprite->stock->nimage; i++) {
    image = sprite->stock->image[i];

    if (image != NULL)
      size += image_line_size(image, image->w) * image->h;
  }

  return size;
}

static Layer *index2layer(const Layer *layer, int index, int *index_count)
{
  if (index == *index_count)
    return (Layer*)layer;
  else {
    (*index_count)++;

    if (layer->is_folder()) {
      Layer *found;

      LayerConstIterator it = static_cast<const LayerFolder*>(layer)->get_layer_begin();
      LayerConstIterator end = static_cast<const LayerFolder*>(layer)->get_layer_end();

      for (; it != end; ++it) {
	if ((found = index2layer(*it, index, index_count)))
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

    if (layer->is_folder()) {
      int found;

      LayerConstIterator it = static_cast<const LayerFolder*>(layer)->get_layer_begin();
      LayerConstIterator end = static_cast<const LayerFolder*>(layer)->get_layer_end();

      for (; it != end; ++it) {
	if ((found = layer2index(*it, find_layer, index_count)) >= 0)
	  return found;
      }
    }

    return -1;
  }
}

static int layer_count_layers(const Layer* layer)
{
  int count = 1;

  if (layer->is_folder()) {
    LayerConstIterator it = static_cast<const LayerFolder*>(layer)->get_layer_begin();
    LayerConstIterator end = static_cast<const LayerFolder*>(layer)->get_layer_end();

    for (; it != end; ++it)
      count += layer_count_layers(*it);
  }

  return count;
}

/**
 * Makes a copy "sprite" without the layers (only with the empty layer set)
 */
static Sprite* general_copy(const Sprite* src_sprite)
{
  Sprite* dst_sprite;
  JLink link;

  dst_sprite = sprite_new(src_sprite->imgtype, src_sprite->w, src_sprite->h);
  if (!dst_sprite)
    return NULL;

  /* copy stock */
  stock_free(dst_sprite->stock);
  dst_sprite->stock = stock_new_copy(src_sprite->stock);
  if (!dst_sprite->stock) {
    delete dst_sprite;
    return NULL;
  }

  /* copy general properties */
  strcpy(dst_sprite->filename, src_sprite->filename);
  sprite_set_frames(dst_sprite, src_sprite->frames);
  memcpy(dst_sprite->frlens, src_sprite->frlens, sizeof(int)*src_sprite->frames);

  /* copy color palettes */
  JI_LIST_FOR_EACH(src_sprite->palettes, link) {
    Palette* pal = reinterpret_cast<Palette*>(link->data);
    sprite_set_palette(dst_sprite, pal, true);
  }

  /* copy path */
  if (dst_sprite->path) {
    path_free(dst_sprite->path);
    dst_sprite->path = NULL;
  }

  if (src_sprite->path) {
    dst_sprite->path = path_new_copy(src_sprite->path);
    if (!dst_sprite->path) {
      delete dst_sprite;
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
      delete dst_sprite;
      return NULL;
    }
  }

  /* copy repositories */
  JI_LIST_FOR_EACH(src_sprite->repository.paths, link) {
    Path* path_copy = path_new_copy(reinterpret_cast<Path*>(link->data));
    if (path_copy)
      sprite_add_path(dst_sprite, path_copy);
  }

  JI_LIST_FOR_EACH(src_sprite->repository.masks, link) {
    Mask* mask_copy = mask_new_copy(reinterpret_cast<Mask*>(link->data));
    if (mask_copy)
      sprite_add_mask(dst_sprite, mask_copy);
  }

  /* copy preferred edition options */
  dst_sprite->preferred.scroll_x = src_sprite->preferred.scroll_x;
  dst_sprite->preferred.scroll_y = src_sprite->preferred.scroll_y;
  dst_sprite->preferred.zoom = src_sprite->preferred.zoom;

  return dst_sprite;
}
