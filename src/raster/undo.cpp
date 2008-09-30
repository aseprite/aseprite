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
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <allegro/config.h>

#include "jinete/jlist.h"

#include "raster/cel.h"
#include "raster/dirty.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "raster/undo.h"

/* undo state */
enum {
  DO_UNDO,
  DO_REDO,
};

/* undo types */
enum {
  /* group */
  UNDO_TYPE_OPEN,
  UNDO_TYPE_CLOSE,

  /* data management */
  UNDO_TYPE_DATA,

  /* image management */
  UNDO_TYPE_IMAGE,
  UNDO_TYPE_FLIP,
  UNDO_TYPE_DIRTY,

  /* stock management */
  UNDO_TYPE_ADD_IMAGE,
  UNDO_TYPE_REMOVE_IMAGE,
  UNDO_TYPE_REPLACE_IMAGE,

  /* cel management */
  UNDO_TYPE_ADD_CEL,
  UNDO_TYPE_REMOVE_CEL,

  /* layer management */
  UNDO_TYPE_ADD_LAYER,
  UNDO_TYPE_REMOVE_LAYER,
  UNDO_TYPE_MOVE_LAYER,
  UNDO_TYPE_SET_LAYER,

  /* misc */
  UNDO_TYPE_SET_MASK,
  UNDO_TYPE_SET_FRAMES,
  UNDO_TYPE_SET_FRLEN,
};

typedef struct UndoChunkData UndoChunkData;
typedef struct UndoChunkImage UndoChunkImage;
typedef struct UndoChunkFlip UndoChunkFlip;
typedef struct UndoChunkDirty UndoChunkDirty;
typedef struct UndoChunkAddImage UndoChunkAddImage;
typedef struct UndoChunkRemoveImage UndoChunkRemoveImage;
typedef struct UndoChunkReplaceImage UndoChunkReplaceImage;
typedef struct UndoChunkAddCel UndoChunkAddCel;
typedef struct UndoChunkRemoveCel UndoChunkRemoveCel;
typedef struct UndoChunkAddLayer UndoChunkAddLayer;
typedef struct UndoChunkRemoveLayer UndoChunkRemoveLayer;
typedef struct UndoChunkMoveLayer UndoChunkMoveLayer;
typedef struct UndoChunkSetLayer UndoChunkSetLayer;
typedef struct UndoChunkSetMask UndoChunkSetMask;
typedef struct UndoChunkSetFrames UndoChunkSetFrames;
typedef struct UndoChunkSetFrlen UndoChunkSetFrlen;

typedef struct UndoChunk
{
  int type;
  int size;
  const char *label;
} UndoChunk;

typedef struct UndoStream
{
  Undo *undo;
  JList chunks;
  int size;
} UndoStream;

typedef struct UndoAction
{
  const char *name;
  void (*invert)(UndoStream *stream, UndoChunk *chunk, int state);
} UndoAction;

static void run_undo(Undo *undo, int state, int discard);
static int count_undo_groups(UndoStream *undo_stream);
static void update_undo(Undo *undo);

/* Undo actions */

static void chunk_open_new(UndoStream *stream);
static void chunk_open_invert(UndoStream *stream, UndoChunk *chunk, int state);

static void chunk_close_new(UndoStream *stream);
static void chunk_close_invert(UndoStream *stream, UndoChunk *chunk, int state);

static void chunk_data_new(UndoStream *stream, GfxObj *gfxobj, void *data, int size);
static void chunk_data_invert(UndoStream *stream, UndoChunkData *chunk, int state);

static void chunk_image_new(UndoStream *stream, Image *image, int x, int y, int w, int h);
static void chunk_image_invert(UndoStream *stream, UndoChunkImage *chunk, int state);

static void chunk_flip_new(UndoStream *stream, Image *image, int x1, int y1, int x2, int y2, int horz);
static void chunk_flip_invert(UndoStream *stream, UndoChunkFlip *chunk, int state);

static void chunk_dirty_new(UndoStream *stream, Dirty *dirty);
static void chunk_dirty_invert(UndoStream *stream, UndoChunkDirty *chunk, int state);

static void chunk_add_image_new(UndoStream *stream, Stock *stock, int image_index);
static void chunk_add_image_invert(UndoStream *stream, UndoChunkAddImage *chunk, int state);

static void chunk_remove_image_new(UndoStream *stream, Stock *stock, int image_index);
static void chunk_remove_image_invert(UndoStream *stream, UndoChunkRemoveImage *chunk, int state);

static void chunk_replace_image_new(UndoStream *stream, Stock *stock, int image_index);
static void chunk_replace_image_invert(UndoStream *stream, UndoChunkReplaceImage *chunk, int state);

static void chunk_add_cel_new(UndoStream *stream, Layer *layer, Cel *cel);
static void chunk_add_cel_invert(UndoStream *stream, UndoChunkAddCel *chunk, int state);

static void chunk_remove_cel_new(UndoStream *stream, Layer *layer, Cel *cel);
static void chunk_remove_cel_invert(UndoStream *stream, UndoChunkRemoveCel *chunk, int state);

static void chunk_add_layer_new(UndoStream *stream, Layer *set, Layer *layer);
static void chunk_add_layer_invert(UndoStream *stream, UndoChunkAddLayer *chunk, int state);

static void chunk_remove_layer_new(UndoStream *stream, Layer *layer);
static void chunk_remove_layer_invert(UndoStream *stream, UndoChunkRemoveLayer *chunk, int state);

static void chunk_move_layer_new(UndoStream *stream, Layer *layer);
static void chunk_move_layer_invert(UndoStream *stream, UndoChunkMoveLayer *chunk, int state);

static void chunk_set_layer_new(UndoStream *stream, Sprite *sprite);
static void chunk_set_layer_invert(UndoStream *stream, UndoChunkSetLayer *chunk, int state);

static void chunk_set_mask_new(UndoStream *stream, Sprite *sprite);
static void chunk_set_mask_invert(UndoStream *stream, UndoChunkSetMask *chunk, int state);

static void chunk_set_frames_new(UndoStream *stream, Sprite *sprite);
static void chunk_set_frames_invert(UndoStream *stream, UndoChunkSetFrames *chunk, int state);

static void chunk_set_frlen_new(UndoStream *stream, Sprite *sprite, int frame);
static void chunk_set_frlen_invert(UndoStream *stream, UndoChunkSetFrlen *chunk, int state);

#define DECL_UNDO_ACTION(name) \
  { #name, (void (*)(UndoStream *, UndoChunk *, int))chunk_##name##_invert }

/* warning: in the same order as in UNDO_TYPEs */
static UndoAction undo_actions[] = {
  DECL_UNDO_ACTION(open),
  DECL_UNDO_ACTION(close),
  DECL_UNDO_ACTION(data),
  DECL_UNDO_ACTION(image),
  DECL_UNDO_ACTION(flip),
  DECL_UNDO_ACTION(dirty),
  DECL_UNDO_ACTION(add_image),
  DECL_UNDO_ACTION(remove_image),
  DECL_UNDO_ACTION(replace_image),
  DECL_UNDO_ACTION(add_cel),
  DECL_UNDO_ACTION(remove_cel),
  DECL_UNDO_ACTION(add_layer),
  DECL_UNDO_ACTION(remove_layer),
  DECL_UNDO_ACTION(move_layer),
  DECL_UNDO_ACTION(set_layer),
  DECL_UNDO_ACTION(set_mask),
  DECL_UNDO_ACTION(set_frames),
  DECL_UNDO_ACTION(set_frlen)
};

/* UndoChunk */

static UndoChunk *undo_chunk_new(UndoStream *stream, int type, int size);
static void undo_chunk_free(UndoChunk *chunk);

/* Raw data */

static Dirty *read_raw_dirty(ase_uint8 *raw_data);
static ase_uint8 *write_raw_dirty(ase_uint8 *raw_data, Dirty *dirty);
static int get_raw_dirty_size(Dirty *dirty);

static Image *read_raw_image(ase_uint8 *raw_data);
static ase_uint8 *write_raw_image(ase_uint8 *raw_data, Image *image);
static int get_raw_image_size(Image *image);

static Cel *read_raw_cel(ase_uint8 *raw_data);
static ase_uint8 *write_raw_cel(ase_uint8 *raw_data, Cel *cel);
static int get_raw_cel_size(Cel *cel);

static Layer *read_raw_layer(ase_uint8 *raw_data);
static ase_uint8 *write_raw_layer(ase_uint8 *raw_data, Layer *layer);
static int get_raw_layer_size(Layer *layer);

static Mask *read_raw_mask(ase_uint8 *raw_data);
static ase_uint8 *write_raw_mask(ase_uint8 *raw_data, Mask *mask);
static int get_raw_mask_size(Mask *mask);

/* UndoStream */

static UndoStream *undo_stream_new(Undo *undo);
static void undo_stream_free(UndoStream *stream);

static UndoChunk *undo_stream_pop_chunk(UndoStream *stream, int tail);
static void undo_stream_push_chunk(UndoStream *stream, UndoChunk *chunk);

/* General undo routines */

Undo *undo_new(Sprite *sprite)
{
  Undo *undo = (Undo *)gfxobj_new(GFXOBJ_UNDO, sizeof(Undo));
  if (!undo)
    return NULL;

  undo->sprite = sprite;
  undo->undo_stream = undo_stream_new(undo);
  undo->redo_stream = undo_stream_new(undo);
  undo->diff_count = 0;
  undo->diff_saved = 0;
  undo->enabled = TRUE;
  undo->label = NULL;

  return undo;
}

void undo_free(Undo *undo)
{
  undo_stream_free(undo->undo_stream);
  undo_stream_free(undo->redo_stream);

  jfree(undo);
}

int undo_get_memsize(Undo *undo)
{
  return
    undo->undo_stream->size +
    undo->redo_stream->size;
}

void undo_enable(Undo *undo)
{
  undo->enabled = TRUE;
}

void undo_disable(Undo *undo)
{
  undo->enabled = FALSE;
}

bool undo_is_enabled(Undo *undo)
{
  return undo->enabled ? TRUE: FALSE;
}

bool undo_is_disabled(Undo *undo)
{
  return !undo_is_enabled(undo);
}

bool undo_can_undo(Undo *undo)
{
  return !jlist_empty(undo->undo_stream->chunks);
}

bool undo_can_redo(Undo *undo)
{
  return !jlist_empty(undo->redo_stream->chunks);
}

void undo_undo(Undo *undo)
{
  run_undo(undo, DO_UNDO, FALSE);
}

void undo_redo(Undo *undo)
{
  run_undo(undo, DO_REDO, FALSE);
}

void undo_set_label(Undo *undo, const char *label)
{
  undo->label = label;
}

const char *undo_get_next_undo_label(Undo *undo)
{
  UndoChunk *chunk;

  assert(undo_can_undo(undo));

  chunk = reinterpret_cast<UndoChunk*>(jlist_first_data(undo->undo_stream->chunks));
  return chunk->label;
}

const char *undo_get_next_redo_label(Undo *undo)
{
  UndoChunk *chunk;

  assert(undo_can_redo(undo));

  chunk = reinterpret_cast<UndoChunk*>(jlist_first_data(undo->redo_stream->chunks));
  return chunk->label;
}

static void run_undo(Undo *undo, int state, int discard_tail)
{
  UndoStream *undo_stream = ((state == DO_UNDO)? undo->undo_stream:
						 undo->redo_stream);
  UndoStream *redo_stream = ((state == DO_REDO)? undo->undo_stream:
						 undo->redo_stream);
  UndoChunk *chunk;
  int level = 0;

  if (!discard_tail) {
    do {
      chunk = undo_stream_pop_chunk(undo_stream, FALSE); /* read from head */
      if (!chunk)
	break;

      { int c;
	for (c=0; c<ABS(level); c++)
	  PRINTF("  ");
	PRINTF("%s: %s (Label: %s)\n",
	       (state == DO_UNDO) ? "Undo": "Redo",
	       undo_actions[chunk->type].name,
	       chunk->label); }

      undo_set_label(undo, chunk->label);
      (undo_actions[chunk->type].invert)(redo_stream, chunk, state);

      if (chunk->type == UNDO_TYPE_OPEN)
	level++;
      else if (chunk->type == UNDO_TYPE_CLOSE)
	level--;

      undo_chunk_free(chunk);

      if (state == DO_UNDO)
	undo->diff_count--;
      else if (state == DO_REDO)
	undo->diff_count++;
    } while (level);
  }
  else {
    do {
      chunk = undo_stream_pop_chunk(undo_stream, TRUE); /* read from tail */
      if (!chunk)
	break;

      if (chunk->type == UNDO_TYPE_OPEN)
	level++;
      else if (chunk->type == UNDO_TYPE_CLOSE)
	level--;

      undo_chunk_free(chunk);
    } while (level);
  }
}

static int count_undo_groups(UndoStream *undo_stream)
{
  UndoChunk *chunk;
  int groups = 0;
  int level;
  JLink link;

  link = jlist_first(undo_stream->chunks);
  while (link != undo_stream->chunks->end) {
    level = 0;

    do {
      chunk = reinterpret_cast<UndoChunk*>(link->data);
      link = link->next;

      if (chunk->type == UNDO_TYPE_OPEN)
	level++;
      else if (chunk->type == UNDO_TYPE_CLOSE)
	level--;
    } while (level && (link != undo_stream->chunks->end));

    if (level == 0)
      groups++;
  }

  return groups;
}

/* called every time a new undo is added */
static void update_undo(Undo *undo)
{
  int groups = count_undo_groups(undo->undo_stream);
  int undo_size_limit = get_config_int("Options", "UndoSizeLimit", 8)*1024*1024;

  /* more diff */
  undo->diff_count++;

  /* reset the "redo" stream */
  if (!jlist_empty(undo->redo_stream->chunks)) {
    undo_stream_free(undo->redo_stream);
    undo->redo_stream = undo_stream_new(undo);
  }

  /* "undo" is too big? */
  while (groups > 1 && undo->undo_stream->size > undo_size_limit) {
    run_undo(undo, DO_UNDO, TRUE);
    groups--;
  }
}

/***********************************************************************

  "open"

     no data

***********************************************************************/

void undo_open(Undo *undo)
{
  chunk_open_new(undo->undo_stream);
  update_undo(undo);
}

static void chunk_open_new(UndoStream *stream)
{
  undo_chunk_new(stream,
		 UNDO_TYPE_OPEN,
		 sizeof(UndoChunk));
}

static void chunk_open_invert(UndoStream *stream, UndoChunk *chunk, int state)
{
  chunk_close_new(stream);
}

/***********************************************************************

  "close"

     no data

***********************************************************************/

void undo_close(Undo *undo)
{
  chunk_close_new(undo->undo_stream);
  update_undo(undo);
}

static void chunk_close_new(UndoStream *stream)
{
  undo_chunk_new(stream,
		 UNDO_TYPE_CLOSE,
		 sizeof(UndoChunk));
}

static void chunk_close_invert(UndoStream *stream, UndoChunk *chunk, int state)
{
  chunk_open_new(stream);
}

/***********************************************************************

  "data"

     DWORD		object ID
     DWORD		data address offset
     DWORD		data size
     BYTE[]		data bytes

***********************************************************************/

struct UndoChunkData
{
  UndoChunk head;
  ase_uint32 gfxobj_id;
  ase_uint32 dataoffset;
  ase_uint32 datasize;
  ase_uint8 data[0];
};

void undo_data(Undo *undo, GfxObj *gfxobj, void *data, int size)
{
  chunk_data_new(undo->undo_stream, gfxobj, data, size);
  update_undo(undo);
}

static void chunk_data_new(UndoStream *stream, GfxObj *gfxobj, void *data, int size)
{
  UndoChunkData *chunk;
  ase_uint32 offset = (unsigned int)(((ase_uint8 *)data) -
				     ((ase_uint8 *)gfxobj));

  assert(size >= 1);

  chunk = (UndoChunkData *)
    undo_chunk_new(stream,
		   UNDO_TYPE_DATA,
		   sizeof(UndoChunkData)+size);

  chunk->gfxobj_id = gfxobj->id;
  chunk->dataoffset = offset;
  chunk->datasize = size;

  memcpy(chunk->data, data, size);
}

static void chunk_data_invert(UndoStream *stream, UndoChunkData *chunk, int state)
{
  unsigned int id = chunk->gfxobj_id;
  unsigned int offset = chunk->dataoffset;
  unsigned int size = chunk->datasize;
  GfxObj *gfxobj = gfxobj_find(id);

  if (gfxobj) {
    void *data = (void *)(((ase_uint8 *)gfxobj) + offset);

    /* save the current data */
    chunk_data_new(stream, gfxobj, data, size);

    /* copy back the old data */
    memcpy(data, chunk->data, size);
  }
}

/***********************************************************************

  "image"

     DWORD		image ID
     BYTE		image type
     WORD[4]		x, y, w, h
     for each row ("h" times)
       for each pixel ("w" times)
	  BYTE[4]	for RGB images, or
	  BYTE[2]	for Grayscale images, or
	  BYTE		for Indexed images

***********************************************************************/

struct UndoChunkImage
{
  UndoChunk head;
  ase_uint32 image_id;
  ase_uint8 imgtype;
  ase_uint16 x, y, w, h; 
  ase_uint8 data[0];
};

void undo_image(Undo *undo, Image *image, int x, int y, int w, int h)
{
  chunk_image_new(undo->undo_stream, image, x, y, w, h);
  update_undo(undo);
}

static void chunk_image_new(UndoStream *stream, Image *image, int x, int y, int w, int h)
{
  UndoChunkImage *chunk;
  ase_uint8 *ptr;
  int v, size;

  assert(w >= 1 && h >= 1);
  assert(x >= 0 && y >= 0 && x+w <= image->w && y+h <= image->h);
  
  size = IMAGE_LINE_SIZE(image, w);

  chunk = (UndoChunkImage *)
    undo_chunk_new(stream,
		   UNDO_TYPE_IMAGE,
		   sizeof(UndoChunkImage) + size*h);
  
  chunk->image_id = image->gfxobj.id;
  chunk->imgtype = image->imgtype;
  chunk->x = x;
  chunk->y = y;
  chunk->w = w;
  chunk->h = h;
/*   chunk->data = jmalloc(size*h); */

  ptr = chunk->data;
  for (v=0; v<h; ++v) {
    memcpy(ptr, IMAGE_ADDRESS(image, x, y+v), size);
    ptr += size;
  }
}

/* static void chunk_image_free(UndoChunkImage *chunk) */
/* { */
/*   jfree(chunk->data); */
/* } */

static void chunk_image_invert(UndoStream *stream, UndoChunkImage *chunk, int state)
{
  unsigned int id = chunk->image_id;
  int imgtype = chunk->imgtype;
  Image *image = (Image *)gfxobj_find(id);

  if ((image) && (image->gfxobj.type == GFXOBJ_IMAGE) &&
      (image->imgtype == imgtype)) {
    int x, y, w, h;
    ase_uint8 *ptr;
    int v, size;

    x = chunk->x;
    y = chunk->y;
    w = chunk->w;
    h = chunk->h;

    /* backup the current image portion */
    chunk_image_new(stream, image, x, y, w, h);

    /* restore the old image portion */
    size = IMAGE_LINE_SIZE(image, chunk->w);
    ptr = chunk->data;

    for (v=0; v<h; ++v) {
      memcpy(IMAGE_ADDRESS(image, x, y+v), ptr, size);
      ptr += size;
    }
  }
}

/***********************************************************************

  "flip"

     DWORD		image ID
     BYTE		image type
     WORD[4]		x1, y1, x2, y2
     BYTE		1=horizontal 0=vertical

***********************************************************************/

struct UndoChunkFlip
{
  UndoChunk head;
  ase_uint32 image_id;
  ase_uint8 imgtype;
  ase_uint16 x1, y1, x2, y2; 
  ase_uint8 horz;
};

void undo_flip(Undo *undo, Image *image, int x1, int y1, int x2, int y2, int horz)
{
  chunk_flip_new(undo->undo_stream, image, x1, y1, x2, y2, horz);
  update_undo(undo);
}

static void chunk_flip_new(UndoStream *stream, Image *image, int x1, int y1, int x2, int y2, int horz)
{
  UndoChunkFlip *chunk = (UndoChunkFlip *)
    undo_chunk_new(stream,
		   UNDO_TYPE_FLIP,
		   sizeof(UndoChunkFlip));

  chunk->image_id = image->gfxobj.id;
  chunk->imgtype = image->imgtype;
  chunk->x1 = x1;
  chunk->y1 = y1;
  chunk->x2 = x2;
  chunk->y2 = y2;
  chunk->horz = horz;
}

static void chunk_flip_invert(UndoStream *stream, UndoChunkFlip *chunk, int state)
{
  Image *image = (Image *)gfxobj_find(chunk->image_id);

  if ((image) &&
      (image->gfxobj.type == GFXOBJ_IMAGE) &&
      (image->imgtype == chunk->imgtype)) {
    int x1, y1, x2, y2;
    int x, y, horz;
    Image *area;

    x1 = chunk->x1;
    y1 = chunk->y1;
    x2 = chunk->x2;
    y2 = chunk->y2;
    horz = chunk->horz;

    chunk_flip_new(stream, image, x1, y1, x2, y2, horz);

    area = image_crop(image, x1, y1, x2-x1+1, y2-y1+1, 0);
    for (y=0; y<(y2-y1+1); y++)
      for (x=0; x<(x2-x1+1); x++)
	image_putpixel(image,
		       horz ? x2-x: x1+x,
		       !horz? y2-y: y1+y,
		       image_getpixel(area, x, y));
    image_free(area);
  }
}

/***********************************************************************

  "dirty"

     DIRTY_DATA		see read/write_raw_dirty

***********************************************************************/

struct UndoChunkDirty
{
  UndoChunk head;
  ase_uint8 data[0];
};

void undo_dirty(Undo *undo, Dirty *dirty)
{
  chunk_dirty_new(undo->undo_stream, dirty);
  update_undo(undo);
}

static void chunk_dirty_new(UndoStream *stream, Dirty *dirty)
{
  UndoChunkDirty *chunk = (UndoChunkDirty *)
    undo_chunk_new(stream,
		   UNDO_TYPE_DIRTY,
		   sizeof(UndoChunkDirty)+get_raw_dirty_size(dirty));

  write_raw_dirty(chunk->data, dirty);
}

static void chunk_dirty_invert(UndoStream *stream, UndoChunkDirty *chunk, int state)
{
  Dirty *dirty = read_raw_dirty(chunk->data);

  if (dirty != NULL) {
    dirty_swap(dirty);
    chunk_dirty_new(stream, dirty);
    dirty_free(dirty);
  }
}

/***********************************************************************

  "add_image"

     DWORD		stock ID
     DWORD		index of the image in the stock

***********************************************************************/

struct UndoChunkAddImage
{
  UndoChunk head;
  ase_uint32 stock_id;
  ase_uint32 image_index;
};

void undo_add_image(Undo *undo, Stock *stock, int image_index)
{
  chunk_add_image_new(undo->undo_stream, stock, image_index);
  update_undo(undo);
}

static void chunk_add_image_new(UndoStream *stream, Stock *stock, int image_index)
{
  UndoChunkAddImage *chunk = (UndoChunkAddImage *)
    undo_chunk_new(stream,
		   UNDO_TYPE_ADD_IMAGE,
		   sizeof(UndoChunkAddImage));

  chunk->stock_id = stock->gfxobj.id;
  chunk->image_index = image_index;
}

static void chunk_add_image_invert(UndoStream *stream, UndoChunkAddImage *chunk, int state)
{
  unsigned int stock_id = chunk->stock_id;
  unsigned int image_index = chunk->image_index;
  Stock *stock = (Stock *)gfxobj_find(stock_id);

  if (stock) {
    Image *image = stock_get_image(stock, image_index);
    if (image != NULL) {
      chunk_remove_image_new(stream, stock, image_index);
      stock_remove_image(stock, image);
      image_free(image);
    }
  }
}

/***********************************************************************

  "remove_image"

     DWORD		stock ID
     DWORD		index of the image in the stock
     IMAGE_DATA		see read/write_raw_image

***********************************************************************/

struct UndoChunkRemoveImage
{
  UndoChunk head;
  ase_uint32 stock_id;
  ase_uint32 image_index;
  ase_uint8 data[0];
};

void undo_remove_image(Undo *undo, Stock *stock, int image_index)
{
  chunk_remove_image_new(undo->undo_stream, stock, image_index);
  update_undo(undo);
}

static void chunk_remove_image_new(UndoStream *stream, Stock *stock, int image_index)
{
  Image *image = stock->image[image_index];
  UndoChunkRemoveImage *chunk = (UndoChunkRemoveImage *)
    undo_chunk_new(stream,
		   UNDO_TYPE_REMOVE_IMAGE,
		   sizeof(UndoChunkRemoveImage)+get_raw_image_size(image));

  chunk->stock_id = stock->gfxobj.id;
  chunk->image_index = image_index;

  write_raw_image(chunk->data, image);
}

static void chunk_remove_image_invert(UndoStream *stream, UndoChunkRemoveImage *chunk, int state)
{
  unsigned int stock_id = chunk->stock_id;
  unsigned int image_index = chunk->image_index;
  Stock *stock = (Stock *)gfxobj_find(stock_id);

  if (stock) {
    Image *image = read_raw_image(chunk->data);

    /* assert(image != NULL); */

    stock_replace_image(stock, image_index, image);
    chunk_add_image_new(stream, stock, image_index);
  }
}

/***********************************************************************

  "replace_image"

     DWORD		stock ID
     DWORD		index of the image in the stock
     IMAGE_DATA		see read/write_raw_image

***********************************************************************/

struct UndoChunkReplaceImage
{
  UndoChunk head;
  ase_uint32 stock_id;
  ase_uint32 image_index;
  ase_uint8 data[0];
};

void undo_replace_image(Undo *undo, Stock *stock, int image_index)
{
  chunk_replace_image_new(undo->undo_stream, stock, image_index);
  update_undo(undo);
}

static void chunk_replace_image_new(UndoStream *stream, Stock *stock, int image_index)
{
  Image *image = stock->image[image_index];
  UndoChunkReplaceImage *chunk = (UndoChunkReplaceImage *)
    undo_chunk_new(stream,
		   UNDO_TYPE_REPLACE_IMAGE,
		   sizeof(UndoChunkReplaceImage)+get_raw_image_size(image));

  chunk->stock_id = stock->gfxobj.id;
  chunk->image_index = image_index;

  write_raw_image(chunk->data, image);
}

static void chunk_replace_image_invert(UndoStream *stream, UndoChunkReplaceImage *chunk, int state)
{
  unsigned long stock_id = chunk->stock_id;
  unsigned long image_index = chunk->image_index;
  Stock *stock = (Stock *)gfxobj_find(stock_id);

  if (stock) {
    Image *image = read_raw_image(chunk->data);

    chunk_replace_image_new(stream, stock, image_index);

    if (stock->image[image_index])
      image_free(stock->image[image_index]);

    stock_replace_image(stock, image_index, image);
  }
}

/***********************************************************************

  "add_cel"

     DWORD		layer ID
     DWORD		cel ID

***********************************************************************/

struct UndoChunkAddCel
{
  UndoChunk head;
  ase_uint32 layer_id;
  ase_uint32 cel_id;
};

void undo_add_cel(Undo *undo, Layer *layer, Cel *cel)
{
  chunk_add_cel_new(undo->undo_stream, layer, cel);
  update_undo(undo);
}

static void chunk_add_cel_new(UndoStream *stream, Layer *layer, Cel *cel)
{
  UndoChunkAddCel *chunk = (UndoChunkAddCel *)
    undo_chunk_new(stream,
		   UNDO_TYPE_ADD_CEL,
		   sizeof(UndoChunkAddCel));

  chunk->layer_id = layer->gfxobj.id;
  chunk->cel_id = cel->gfxobj.id;
}

static void chunk_add_cel_invert(UndoStream *stream, UndoChunkAddCel *chunk, int state)
{
  Layer *layer = (Layer *)gfxobj_find(chunk->layer_id);
  Cel *cel = (Cel *)gfxobj_find(chunk->cel_id);

  if (layer && cel) {
    chunk_remove_cel_new(stream, layer, cel);

    layer_remove_cel(layer, cel);
    cel_free(cel);
  }
}

/***********************************************************************

  "remove_cel"

     DWORD		layer ID
     CEL_DATA		see read/write_raw_cel

***********************************************************************/

struct UndoChunkRemoveCel
{
  UndoChunk head;
  ase_uint32 layer_id;
  ase_uint8 data[0];
};

void undo_remove_cel(Undo *undo, Layer *layer, Cel *cel)
{
  chunk_remove_cel_new(undo->undo_stream, layer, cel);
  update_undo(undo);
}

static void chunk_remove_cel_new(UndoStream *stream, Layer *layer, Cel *cel)
{
  UndoChunkRemoveCel *chunk = (UndoChunkRemoveCel *)
    undo_chunk_new(stream,
		   UNDO_TYPE_REMOVE_CEL,
		   sizeof(UndoChunkRemoveCel)+get_raw_cel_size(cel));

  chunk->layer_id = layer->gfxobj.id;
  write_raw_cel(chunk->data, cel);
}

static void chunk_remove_cel_invert(UndoStream *stream, UndoChunkRemoveCel *chunk, int state)
{
  unsigned int layer_id = chunk->layer_id;
  Layer *layer = (Layer *)gfxobj_find(layer_id);

  if (layer) {
    Cel *cel = read_raw_cel(chunk->data);

    /* assert(cel != NULL); */

    chunk_add_cel_new(stream, layer, cel);
    layer_add_cel(layer, cel);
  }
}

/***********************************************************************

  "add_layer"

     DWORD		parent layer set ID
     DWORD		layer ID

***********************************************************************/

struct UndoChunkAddLayer
{
  UndoChunk head;
  ase_uint32 set_id;
  ase_uint32 layer_id;
};

void undo_add_layer(Undo *undo, Layer *set, Layer *layer)
{
  chunk_add_layer_new(undo->undo_stream, set, layer);
  update_undo(undo);
}

static void chunk_add_layer_new(UndoStream *stream, Layer *set, Layer *layer)
{
  UndoChunkAddLayer *chunk = (UndoChunkAddLayer *)
    undo_chunk_new(stream,
		   UNDO_TYPE_ADD_LAYER,
		   sizeof(UndoChunkAddLayer));

  chunk->set_id = set->gfxobj.id;
  chunk->layer_id = layer->gfxobj.id;
}

static void chunk_add_layer_invert(UndoStream *stream, UndoChunkAddLayer *chunk, int state)
{
  Layer *set = (Layer *)gfxobj_find(chunk->set_id);
  Layer *layer = (Layer *)gfxobj_find(chunk->layer_id);

  if (set && layer) {
    chunk_remove_layer_new(stream, layer);

    layer_remove_layer(set, layer);
    layer_free_images(layer);
    layer_free(layer);
  }
}

/***********************************************************************

  "remove_layer"

     DWORD		parent layer set ID
     DWORD		after layer ID
     LAYER_DATA		see read/write_raw_layer

***********************************************************************/

struct UndoChunkRemoveLayer
{
  UndoChunk head;
  ase_uint32 set_id;
  ase_uint32 after_id;
  ase_uint8 data[0];
};

void undo_remove_layer(Undo *undo, Layer *layer)
{
  chunk_remove_layer_new(undo->undo_stream, layer);
  update_undo(undo);
}

static void chunk_remove_layer_new(UndoStream *stream, Layer *layer)
{
  UndoChunkRemoveLayer *chunk = (UndoChunkRemoveLayer *)
    undo_chunk_new(stream,
		   UNDO_TYPE_REMOVE_LAYER,
		   sizeof(UndoChunkRemoveLayer)+get_raw_layer_size(layer));
  Layer *set = layer->parent_layer;
  Layer *after = layer_get_prev(layer);

  chunk->set_id = set->gfxobj.id;
  chunk->after_id = after != NULL ? after->gfxobj.id: 0;

  write_raw_layer(chunk->data, layer);
}

static void chunk_remove_layer_invert(UndoStream *stream, UndoChunkRemoveLayer *chunk, int state)
{
  Layer *set = (Layer *)gfxobj_find(chunk->set_id);
  Layer *after = (Layer *)gfxobj_find(chunk->after_id);

  if (set != NULL) {
    Layer *layer = read_raw_layer(chunk->data);

    /* assert(layer != NULL); */

    chunk_add_layer_new(stream, set, layer);
    layer_add_layer(set, layer);
    layer_move_layer(set, layer, after);
  }
}

/***********************************************************************

  "move_layer"

     DWORD		parent layer set ID
     DWORD		layer ID
     DWORD		after layer ID

***********************************************************************/

struct UndoChunkMoveLayer
{
  UndoChunk head;
  ase_uint32 set_id;
  ase_uint32 layer_id;
  ase_uint32 after_id;
};

void undo_move_layer(Undo *undo, Layer *layer)
{
  chunk_move_layer_new(undo->undo_stream, layer);
  update_undo(undo);
}

static void chunk_move_layer_new(UndoStream *stream, Layer *layer)
{
  UndoChunkMoveLayer *chunk = (UndoChunkMoveLayer *)
    undo_chunk_new(stream,
		   UNDO_TYPE_MOVE_LAYER,
		   sizeof(UndoChunkMoveLayer));
  Layer *set = layer->parent_layer;
  Layer *after = layer_get_prev(layer);

  chunk->set_id = set->gfxobj.id;
  chunk->layer_id = layer->gfxobj.id;
  chunk->after_id = after ? after->gfxobj.id: 0;
}

static void chunk_move_layer_invert(UndoStream *stream, UndoChunkMoveLayer *chunk, int state)
{
  Layer *set = (Layer *)gfxobj_find(chunk->set_id);
  Layer *layer = (Layer *)gfxobj_find(chunk->layer_id);
  Layer *after = (Layer *)gfxobj_find(chunk->after_id);

  if (set && layer) {
    chunk_move_layer_new(stream, layer);
    layer_move_layer(set, layer, after);
  }
}

/***********************************************************************

  "set_layer"

     DWORD		sprite ID
     DWORD		layer ID

***********************************************************************/

struct UndoChunkSetLayer
{
  UndoChunk head;
  ase_uint32 sprite_id;
  ase_uint32 layer_id;
};

void undo_set_layer(Undo *undo, Sprite *sprite)
{
  chunk_set_layer_new(undo->undo_stream, sprite);
  update_undo(undo);
}

static void chunk_set_layer_new(UndoStream *stream, Sprite *sprite)
{
  UndoChunkSetLayer *chunk = (UndoChunkSetLayer *)
    undo_chunk_new(stream,
		   UNDO_TYPE_SET_LAYER,
		   sizeof(UndoChunkSetLayer));

  chunk->sprite_id = sprite->gfxobj.id;
  chunk->layer_id = sprite->layer ? sprite->layer->gfxobj.id: 0;
}

static void chunk_set_layer_invert(UndoStream *stream, UndoChunkSetLayer *chunk, int state)
{
  Sprite *sprite = (Sprite *)gfxobj_find(chunk->sprite_id);
  Layer *layer = (Layer *)gfxobj_find(chunk->layer_id);

  if (sprite) {
    chunk_set_layer_new(stream, sprite);

    sprite->layer = layer;
  }
}

/***********************************************************************

  "set_mask"

     DWORD		sprite ID
     MASK_DATA		see read/write_raw_mask

***********************************************************************/

struct UndoChunkSetMask
{
  UndoChunk head;
  ase_uint32 sprite_id;
  ase_uint8 data[0];
};

void undo_set_mask(Undo *undo, Sprite *sprite)
{
  chunk_set_mask_new(undo->undo_stream, sprite);
  update_undo(undo);
}

static void chunk_set_mask_new(UndoStream *stream, Sprite *sprite)
{
  UndoChunkSetMask *chunk = (UndoChunkSetMask *)
    undo_chunk_new(stream,
		   UNDO_TYPE_SET_MASK,
		   sizeof(UndoChunkSetMask)+get_raw_mask_size(sprite->mask));

  chunk->sprite_id = sprite->gfxobj.id;
  write_raw_mask(chunk->data, sprite->mask);
}

static void chunk_set_mask_invert(UndoStream *stream, UndoChunkSetMask *chunk, int state)
{
  Sprite *sprite = (Sprite *)gfxobj_find(chunk->sprite_id);

  if (sprite) {
    Mask *mask = read_raw_mask(chunk->data);

    chunk_set_mask_new(stream, sprite);
    mask_copy(sprite->mask, mask);

    mask_free(mask);
  }
}

/***********************************************************************

  "set_frames"

     DWORD		sprite ID
     DWORD		frames

***********************************************************************/

struct UndoChunkSetFrames
{
  UndoChunk head;
  ase_uint32 sprite_id;
  ase_uint32 frames;
};

void undo_set_frames(Undo *undo, Sprite *sprite)
{
  chunk_set_frames_new(undo->undo_stream, sprite);
  update_undo(undo);
}

static void chunk_set_frames_new(UndoStream *stream, Sprite *sprite)
{
  UndoChunkSetFrames *chunk = (UndoChunkSetFrames *)
    undo_chunk_new(stream,
		   UNDO_TYPE_SET_FRAMES,
		   sizeof(UndoChunkSetFrames));

  chunk->sprite_id = sprite->gfxobj.id;
  chunk->frames = sprite->frames;
}

static void chunk_set_frames_invert(UndoStream *stream, UndoChunkSetFrames *chunk, int state)
{
  Sprite *sprite = (Sprite *)gfxobj_find(chunk->sprite_id);

  if (sprite) {
    chunk_set_frames_new(stream, sprite);
    sprite_set_frames(sprite, chunk->frames);
  }
}

/***********************************************************************

  "set_frlen"

     DWORD		sprite ID
     DWORD		frame
     DWORD		frlen

***********************************************************************/

struct UndoChunkSetFrlen
{
  UndoChunk head;
  ase_uint32 sprite_id;
  ase_uint32 frame;
  ase_uint32 frlen;
};

void undo_set_frlen(Undo *undo, Sprite *sprite, int frame)
{
  chunk_set_frlen_new(undo->undo_stream, sprite, frame);
  update_undo(undo);
}

static void chunk_set_frlen_new(UndoStream *stream, Sprite *sprite, int frame)
{
  UndoChunkSetFrlen *chunk = (UndoChunkSetFrlen *)
    undo_chunk_new(stream,
		   UNDO_TYPE_SET_FRLEN,
		   sizeof(UndoChunkSetFrlen));

  assert(frame >= 0 && frame < sprite->frames);

  chunk->sprite_id = sprite->gfxobj.id;
  chunk->frame = frame;
  chunk->frlen = sprite->frlens[frame];
}

static void chunk_set_frlen_invert(UndoStream *stream, UndoChunkSetFrlen *chunk, int state)
{
  Sprite *sprite = (Sprite *)gfxobj_find(chunk->sprite_id);

  if (sprite != NULL) {
    chunk_set_frlen_new(stream, sprite, chunk->frame);
    sprite_set_frlen(sprite, chunk->frame, chunk->frlen);
  }
}

/***********************************************************************

  Helper routines for UndoChunk

***********************************************************************/

static UndoChunk *undo_chunk_new(UndoStream *stream, int type, int size)
{
  UndoChunk *chunk;

  assert(size >= sizeof(UndoChunk));

  chunk = (UndoChunk *)jmalloc0(size);
  if (!chunk)
    return NULL;

  chunk->type = type;
  chunk->size = size;
  chunk->label = stream->undo->label ?
    stream->undo->label:
    undo_actions[chunk->type].name;

  undo_stream_push_chunk(stream, chunk);
  return chunk;
}

static void undo_chunk_free(UndoChunk *chunk)
{
  jfree(chunk);
}

/***********************************************************************

  Raw data

***********************************************************************/

#define read_raw_uint32(dst)		\
  {					\
    memcpy(&dword, raw_data, 4);	\
    dst = dword;			\
    raw_data += 4;			\
  }

#define read_raw_uint16(dst)		\
  {					\
    memcpy(&word, raw_data, 2);		\
    dst = word;				\
    raw_data += 2;			\
  }

#define read_raw_int16(dst)		\
  {					\
    memcpy(&word, raw_data, 2);		\
    dst = (int16_t)word;		\
    raw_data += 2;			\
  }

#define read_raw_uint8(dst)		\
  {					\
    dst = *raw_data;			\
    ++raw_data;				\
  }

#define read_raw_data(dst, size)	\
  {					\
    memcpy(dst, raw_data, size);	\
    raw_data += size;			\
  }

#define write_raw_uint32(src)		\
  {					\
    dword = src;			\
    memcpy(raw_data, &dword, 4);	\
    raw_data += 4;			\
  }

#define write_raw_uint16(src)		\
  {					\
    word = src;				\
    memcpy(raw_data, &word, 2);		\
    raw_data += 2;			\
  }

#define write_raw_int16(src)		\
  {					\
    word = (int16_t)src;		\
    memcpy(raw_data, &word, 2);		\
    raw_data += 2;			\
  }

#define write_raw_uint8(src)		\
  {					\
    *raw_data = src;			\
    ++raw_data;				\
  }

#define write_raw_data(src, size)	\
  {					\
    memcpy(raw_data, src, size);	\
    raw_data += size;			\
  }

/***********************************************************************

  Raw dirty data

     DWORD		image ID
     BYTE		image type
     WORD[4]		x1, y1, x2, y2
     WORD		rows
     for each row
       WORD[2]		y, columns
       for each column
         WORD[2]	x, w
	 for each pixel ("w" times)
	   BYTE[4]	for RGB images, or
	   BYTE[2]	for Grayscale images, or
	   BYTE		for Indexed images

***********************************************************************/

static Dirty *read_raw_dirty(ase_uint8 *raw_data)
{
  ase_uint32 dword;
  ase_uint16 word;
  int x1, y1, x2, y2, size;
  unsigned int image_id;
  int u, v, x, y;
  int imgtype;
  Image *image;
  Dirty *dirty = NULL;

  read_raw_uint32(image_id);
  read_raw_uint8(imgtype);

  image = (Image *)gfxobj_find(image_id);

  if ((image) &&
      (image->gfxobj.type == GFXOBJ_IMAGE) &&
      (image->imgtype == imgtype)) {

    read_raw_uint16(x1);
    read_raw_uint16(y1);
    read_raw_uint16(x2);
    read_raw_uint16(y2);

    dirty = dirty_new(image, x1, y1, x2, y2, FALSE);
    read_raw_uint16(dirty->rows);

    if (dirty->rows > 0) {
      dirty->row = (DirtyRow*)jmalloc(sizeof(DirtyRow) * dirty->rows);

      for (v=0; v<dirty->rows; v++) {
	read_raw_uint16(dirty->row[v].y);
	read_raw_uint16(dirty->row[v].cols);

	y = dirty->row[v].y;
	
	dirty->row[v].col = (DirtyCol*)jmalloc(sizeof(DirtyCol) * dirty->row[v].cols);
	for (u=0; u<dirty->row[v].cols; u++) {
	  read_raw_uint16(dirty->row[v].col[u].x);
	  read_raw_uint16(dirty->row[v].col[u].w);

	  x = dirty->row[v].col[u].x;

	  size = IMAGE_LINE_SIZE(dirty->image, dirty->row[v].col[u].w);

	  dirty->row[v].col[u].flags = DIRTY_VALID_COLUMN;
	  dirty->row[v].col[u].data = jmalloc(size);
	  dirty->row[v].col[u].ptr = IMAGE_ADDRESS(dirty->image, x, y);

	  read_raw_data(dirty->row[v].col[u].data, size);
	}
      }
    }
  }

  return dirty;
}

static ase_uint8 *write_raw_dirty(ase_uint8 *raw_data, Dirty *dirty)
{
  ase_uint32 dword;
  ase_uint16 word;
  int u, v, size;

  write_raw_uint32(dirty->image->gfxobj.id);
  write_raw_uint8(dirty->image->imgtype);
  write_raw_uint16(dirty->x1);
  write_raw_uint16(dirty->y1);
  write_raw_uint16(dirty->x2);
  write_raw_uint16(dirty->y2);
  write_raw_uint16(dirty->rows);

  for (v=0; v<dirty->rows; v++) {
    write_raw_uint16(dirty->row[v].y);
    write_raw_uint16(dirty->row[v].cols);

    for (u=0; u<dirty->row[v].cols; u++) {
      write_raw_uint16(dirty->row[v].col[u].x);
      write_raw_uint16(dirty->row[v].col[u].w);

      size = IMAGE_LINE_SIZE(dirty->image, dirty->row[v].col[u].w);
      write_raw_data(dirty->row[v].col[u].data, size);
    }
  }

  return raw_data;
}

static int get_raw_dirty_size(Dirty *dirty)
{
  int u, v, size = 4+1+2*4+2;	/* DWORD+BYTE+WORD[4]+WORD */

  for (v=0; v<dirty->rows; v++) {
    size += 4;			/* y, cols (WORD[2]) */
    for (u=0; u<dirty->row[v].cols; u++) {
      size += 4;		/* x, w (WORD[2]) */
      size += IMAGE_LINE_SIZE(dirty->image, dirty->row[v].col[u].w);
    }
  }

  return size;
}

/***********************************************************************

  Raw image data

     DWORD		image ID
     BYTE		image type
     WORD[2]		w, h
     for each line	("h" times)
       for each pixel ("w" times)
	 BYTE[4]	for RGB images, or
	 BYTE[2]	for Grayscale images, or
	 BYTE		for Indexed images

***********************************************************************/

static Image *read_raw_image(ase_uint8 *raw_data)
{
  ase_uint32 dword;
  ase_uint16 word;
  gfxobj_id image_id;
  int imgtype;
  int width;
  int height;
  Image *image;
  int c, size;

  read_raw_uint32(image_id);	/* ID */
  if (!image_id)
    return NULL;

  read_raw_uint8(imgtype);	   /* imgtype */
  read_raw_uint16(width);	   /* width */
  read_raw_uint16(height);	   /* height */

  image = image_new(imgtype, width, height);
  size = IMAGE_LINE_SIZE(image, image->w);

  for (c=0; c<image->h; c++)
    read_raw_data(image->line[c], size);

  _gfxobj_set_id((GfxObj *)image, image_id);
  return image;
}

static ase_uint8 *write_raw_image(ase_uint8 *raw_data, Image *image)
{
  ase_uint32 dword;
  ase_uint16 word;
  int c, size;

  write_raw_uint32(image->gfxobj.id);	   /* ID */
  write_raw_uint8(image->imgtype);	   /* imgtype */
  write_raw_uint16(image->w);		   /* width */
  write_raw_uint16(image->h);		   /* height */

  size = IMAGE_LINE_SIZE(image, image->w);
  for (c=0; c<image->h; c++)
    write_raw_data(image->line[c], size);

  return raw_data;
}

static int get_raw_image_size(Image *image)
{
  assert(image != NULL);
  return 4+1+2+2+IMAGE_LINE_SIZE(image, image->w) * image->h;
}

/***********************************************************************

  Raw cel data

     DWORD		cel ID
     WORD		frame
     WORD		image index
     WORD[2]		x, y
     WORD		opacity

***********************************************************************/

static Cel *read_raw_cel(ase_uint8 *raw_data)
{
  ase_uint32 dword;
  ase_uint16 word;
  int frame, image, x, y, opacity;
  gfxobj_id cel_id;
  Cel *cel;

  read_raw_uint32(cel_id);
  read_raw_uint16(frame);
  read_raw_uint16(image);
  read_raw_int16(x);
  read_raw_int16(y);
  read_raw_uint16(opacity);

  cel = cel_new(frame, image);
  cel_set_position(cel, x, y);
  cel_set_opacity(cel, opacity);
  
  _gfxobj_set_id((GfxObj *)cel, cel_id);
  return cel;
}

static ase_uint8 *write_raw_cel(ase_uint8 *raw_data, Cel *cel)
{
  ase_uint32 dword;
  ase_uint16 word;

  write_raw_uint32(cel->gfxobj.id);
  write_raw_uint16(cel->frame);
  write_raw_uint16(cel->image);
  write_raw_int16(cel->x);
  write_raw_int16(cel->y);
  write_raw_uint16(cel->opacity);

  return raw_data;
}

static int get_raw_cel_size(Cel *cel)
{
  return 4+2*5;
}

/***********************************************************************

  Raw layer data

***********************************************************************/

static Layer *read_raw_layer(ase_uint8 *raw_data)
{
  ase_uint32 dword;
  ase_uint16 word;
  gfxobj_id layer_id, sprite_id;
  char name[LAYER_NAME_SIZE];
  int flags, layer_type;
  Layer *layer = NULL;
  Sprite *sprite;

  read_raw_uint32(layer_id);			    /* ID */
  read_raw_data(name, LAYER_NAME_SIZE);		    /* name */
  read_raw_uint8(flags);			    /* flags */
  read_raw_uint16(layer_type);			    /* type */
  read_raw_uint32(sprite_id);			    /* sprite */

  sprite = (Sprite *)gfxobj_find(sprite_id);

  switch (layer_type) {

    case GFXOBJ_LAYER_IMAGE: {
      int c, blend_mode, cels;

      read_raw_uint8(blend_mode); /* blend mode */
      read_raw_uint16(cels);	  /* cels */

      /* create layer */
      layer = layer_new(sprite);

      /* set blend mode */
      layer_set_blend_mode(layer, blend_mode);

      /* read cels */
      for (c=0; c<cels; c++) {
	Cel *cel;
	bool has_image;

	/* read the cel */
	cel = read_raw_cel(raw_data);
	raw_data += get_raw_cel_size(cel);

	/* add the cel in the layer */
	layer_add_cel(layer, cel);

	/* read the image */
	read_raw_uint8(has_image);
	if (has_image) {
	  Image *image = read_raw_image(raw_data);
	  raw_data += get_raw_image_size(image);

	  stock_replace_image(layer->sprite->stock, cel->image, image);
	}
      }
      break;
    }

    case GFXOBJ_LAYER_SET: {
      int c, layers;

      /* create the layer set */
      layer = layer_set_new(sprite);

      /* read how many sub-layers */
      read_raw_uint16(layers);

      for (c=0; c<layers; c++) {
	Layer *child = read_raw_layer(raw_data);
	if (child) {
	  layer_add_layer(layer, child);
	  raw_data += get_raw_layer_size(child);
	}
	else
	  break;
      }
      break;
    }
      
  }

  if (layer != NULL) {
    layer_set_name(layer, name);
    layer->flags = flags;

    _gfxobj_set_id((GfxObj *)layer, layer_id);
  }

  return layer;
}

static ase_uint8 *write_raw_layer(ase_uint8 *raw_data, Layer *layer)
{
  ase_uint32 dword;
  ase_uint16 word;
  JLink link;

  write_raw_uint32(layer->gfxobj.id);		    /* ID */
  write_raw_data(layer->name, LAYER_NAME_SIZE);	    /* name */
  write_raw_uint8(layer->flags);		    /* flags */
  write_raw_uint16(layer->gfxobj.type);		    /* type */
  write_raw_uint32(layer->sprite->gfxobj.id);	    /* sprite */

  switch (layer->gfxobj.type) {

    case GFXOBJ_LAYER_IMAGE:
      /* blend mode */
      write_raw_uint8(layer->blend_mode);
      /* cels */
      write_raw_uint16(jlist_length(layer->cels));
      JI_LIST_FOR_EACH(layer->cels, link) {
	Cel *cel = reinterpret_cast<Cel*>(link->data);
	raw_data = write_raw_cel(raw_data, cel);

	if (cel_is_link(cel, layer)) {
	  write_raw_uint8(0);
	}
	else {
	  Image *image = layer->sprite->stock->image[cel->image];
	  assert(image != NULL);

	  write_raw_uint8(1);
	  raw_data = write_raw_image(raw_data, image);
	}
      }
      break;

    case GFXOBJ_LAYER_SET:
      write_raw_uint16(jlist_length(layer->layers)); /* how many sub-layers */
      JI_LIST_FOR_EACH(layer->layers, link) {
	raw_data = write_raw_layer(raw_data, reinterpret_cast<Layer*>(link->data));
      }
      break;

  }

  return raw_data;
}

static int get_raw_layer_size(Layer *layer)
{
  JLink link;
  int size = 4+LAYER_NAME_SIZE+1+2+4;

  switch (layer->gfxobj.type) {

    case GFXOBJ_LAYER_IMAGE:
      size += 1;		/* blend mode */
      size += 2;		/* num of cels */
      JI_LIST_FOR_EACH(layer->cels, link) {
	Cel *cel = reinterpret_cast<Cel*>(link->data);
	size += get_raw_cel_size(cel);
	size++;			/* has image? */
	if (!cel_is_link(cel, layer)) {
	  Image *image = layer->sprite->stock->image[cel->image];
	  size += get_raw_image_size(image);
	}
      }
      break;

    case GFXOBJ_LAYER_SET:
      size += 2;		/* how many sub-layers */
      JI_LIST_FOR_EACH(layer->layers, link) {
	size += get_raw_layer_size(reinterpret_cast<Layer*>(link->data));
      }
      break;

  }

  return size;
}

/***********************************************************************

  Raw mask data

      WORD[4]		x, y, w, h
      for each line	("h" times)
        for each packet	("((w+7)/8)" times)
          BYTE		8 pixels of the mask

***********************************************************************/

static Mask *read_raw_mask(ase_uint8 *raw_data)
{
  ase_uint16 word;
  int x, y, w, h;
  int c, size;
  Mask *mask;

  read_raw_uint16(x);		/* xpos */
  read_raw_uint16(y);		/* ypos */
  read_raw_uint16(w);		/* width */
  read_raw_uint16(h);		/* height */

  mask = mask_new();
  if (!mask)
    return NULL;

  if (w > 0 && h > 0) {
    size = (w+7)/8;

    mask_union(mask, x, y, w, h);
    for (c=0; c<mask->h; c++)
      read_raw_data(mask->bitmap->line[c], size);
  }

  return mask;
}

static ase_uint8 *write_raw_mask(ase_uint8 *raw_data, Mask *mask)
{
  ase_uint16 word;
  int c, size = (mask->w+7)/8;

  write_raw_uint16(mask->x);	/* xpos */
  write_raw_uint16(mask->y);	/* ypos */
  write_raw_uint16(mask->bitmap ? mask->w: 0); /* width */
  write_raw_uint16(mask->bitmap ? mask->h: 0); /* height */

  if (mask->bitmap)
    for (c=0; c<mask->h; c++)
      write_raw_data(mask->bitmap->line[c], size);

  return raw_data;
}

static int get_raw_mask_size(Mask *mask)
{
  int size = (mask->w+7)/8;

  return 2*4 + (mask->bitmap ? mask->h*size: 0);
}

/***********************************************************************

  Helper routines for UndoStream (a serie of UndoChunks)

***********************************************************************/

static UndoStream *undo_stream_new(Undo *undo)
{
  UndoStream *stream;

  stream = jnew(UndoStream, 1);
  if (!stream)
    return NULL;

  stream->undo = undo;
  stream->chunks = jlist_new();
  stream->size = 0;

  return stream;
}

static void undo_stream_free(UndoStream *stream)
{
  JLink link;

  JI_LIST_FOR_EACH(stream->chunks, link)
    undo_chunk_free(reinterpret_cast<UndoChunk*>(link->data));

  jlist_free(stream->chunks);
  jfree(stream);
}

static UndoChunk *undo_stream_pop_chunk(UndoStream *stream, int tail)
{
  UndoChunk *chunk;
  JLink link;

  if (!jlist_empty(stream->chunks)) {
    if (!tail)
      link = jlist_first(stream->chunks);
    else
      link = jlist_last(stream->chunks);

    chunk = reinterpret_cast<UndoChunk*>(link->data);
    jlist_delete_link(stream->chunks, link);
    stream->size -= chunk->size;
  }
  else
    chunk = NULL;

  return chunk;
}

static void undo_stream_push_chunk(UndoStream *stream, UndoChunk *chunk)
{
  jlist_prepend(stream->chunks, chunk);
  stream->size += chunk->size;
}
