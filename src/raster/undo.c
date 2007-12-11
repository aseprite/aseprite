/* ASE - Allegro Sprite Editor
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

#include <errno.h>
#include <limits.h>
#include <stdio.h>

#include "jinete/jlist.h"

#include "raster/cel.h"
#include "raster/dirty.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "raster/undo.h"

#endif

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
};

typedef struct UndoChunk
{
  int type;
  int max_size;
  int size;
  int pos;
  ase_uint8 *data;
} UndoChunk;

typedef struct UndoStream
{
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

static void chunk_open(UndoStream *stream);
static void chunk_open_invert(UndoStream *stream, UndoChunk *chunk, int state);

static void chunk_close(UndoStream *stream);
static void chunk_close_invert(UndoStream *stream, UndoChunk *chunk, int state);

static void chunk_data(UndoStream *stream, GfxObj *gfxobj, void *data, int size);
static void chunk_data_invert(UndoStream *stream, UndoChunk *chunk, int state);

static void chunk_image(UndoStream *stream, Image *image, int x, int y, int w, int h);
static void chunk_image_invert(UndoStream *stream, UndoChunk *chunk, int state);

static void chunk_flip(UndoStream *stream, Image *image, int x1, int y1, int x2, int y2, int horz);
static void chunk_flip_invert(UndoStream *stream, UndoChunk *chunk, int state);

static void chunk_dirty(UndoStream *stream, Dirty *dirty);
static void chunk_dirty_invert(UndoStream *stream, UndoChunk *chunk, int state);

static void chunk_add_image(UndoStream *stream, Stock *stock, Image *image);
static void chunk_add_image_invert(UndoStream *stream, UndoChunk *chunk, int state);

static void chunk_remove_image(UndoStream *stream, Stock *stock, Image *image);
static void chunk_remove_image_invert(UndoStream *stream, UndoChunk *chunk, int state);

static void chunk_replace_image(UndoStream *stream, Stock *stock, int index);
static void chunk_replace_image_invert(UndoStream *stream, UndoChunk *chunk, int state);

static void chunk_add_cel(UndoStream *stream, Layer *layer, Cel *cel);
static void chunk_add_cel_invert(UndoStream *stream, UndoChunk *chunk, int state);

static void chunk_remove_cel(UndoStream *stream, Layer *layer, Cel *cel);
static void chunk_remove_cel_invert(UndoStream *stream, UndoChunk *chunk, int state);

static void chunk_add_layer(UndoStream *stream, Layer *set, Layer *layer);
static void chunk_add_layer_invert(UndoStream *stream, UndoChunk *chunk, int state);

static void chunk_remove_layer(UndoStream *stream, Layer *layer);
static void chunk_remove_layer_invert(UndoStream *stream, UndoChunk *chunk, int state);

static void chunk_move_layer(UndoStream *stream, Layer *layer);
static void chunk_move_layer_invert(UndoStream *stream, UndoChunk *chunk, int state);

static void chunk_set_layer(UndoStream *stream, Sprite *sprite);
static void chunk_set_layer_invert(UndoStream *stream, UndoChunk *chunk, int state);

static void chunk_set_mask(UndoStream *stream, Sprite *sprite);
static void chunk_set_mask_invert(UndoStream *stream, UndoChunk *chunk, int state);

static void chunk_set_frames(UndoStream *stream, Sprite *sprite);
static void chunk_set_frames_invert(UndoStream *stream, UndoChunk *chunk, int state);

static UndoAction undo_actions[] = { /* in UNDO_TYPEs order */
  { "Open", chunk_open_invert },
  { "Close", chunk_close_invert },
  { "Data", chunk_data_invert },
  { "Image", chunk_image_invert },
  { "Flip", chunk_flip_invert },
  { "Dirty", chunk_dirty_invert },
  { "Add image", chunk_add_image_invert },
  { "Remove image", chunk_remove_image_invert },
  { "Replace image", chunk_replace_image_invert },
  { "Add cel", chunk_add_cel_invert },
  { "Remove cel", chunk_remove_cel_invert },
  { "Add layer", chunk_add_layer_invert },
  { "Remove layer", chunk_remove_layer_invert },
  { "Move layer", chunk_move_layer_invert },
  { "Set layer", chunk_set_layer_invert },
  { "Set mask", chunk_set_mask_invert },
  { "Set frames", chunk_set_frames_invert },
};

/* UndoChunk */

static UndoChunk *undo_chunk_new(int type);
static void undo_chunk_free(UndoChunk *chunk);

static int undo_chunk_get8(UndoChunk *chunk);
static void undo_chunk_put8(UndoChunk *chunk, int c);

static int undo_chunk_get16(UndoChunk *chunk);
static void undo_chunk_put16(UndoChunk *chunk, int c);

static long undo_chunk_get32(UndoChunk *chunk);
static void undo_chunk_put32(UndoChunk *chunk, long c);

/* static double undo_chunk_get64(UndoChunk *chunk); */
/* static void undo_chunk_put64(UndoChunk *chunk, double c); */

static int undo_chunk_read(UndoChunk *chunk, ase_uint8 *buf, int size);
static void undo_chunk_write(UndoChunk *chunk, const ase_uint8 *buf, int size);

static char *undo_chunk_read_string(UndoChunk *chunk);
static void undo_chunk_write_string(UndoChunk *chunk, const char *string);

static Cel *undo_chunk_read_cel(UndoChunk *chunk);
static void undo_chunk_write_cel(UndoChunk *chunk, Cel *cel);

static Layer *undo_chunk_read_layer(UndoChunk *chunk);
static void undo_chunk_write_layer(UndoChunk *chunk, Layer *layer);

static Image *undo_chunk_read_image(UndoChunk *chunk);
static void undo_chunk_write_image(UndoChunk *chunk, Image *image);

static Mask *undo_chunk_read_mask(UndoChunk *chunk);
static void undo_chunk_write_mask(UndoChunk *chunk, Mask *mask);

/* UndoStream */

static UndoStream *undo_stream_new(void);
static void undo_stream_free(UndoStream *stream);

static UndoChunk *undo_stream_pop_chunk(UndoStream *stream, int tail);
static void undo_stream_push_chunk(UndoStream *stream, UndoChunk *chunk);

/* static long undo_stream_raw_read_dword(UndoStream *stream); */
/* static void undo_stream_raw_write_dword(UndoStream *stream, long dword); */

/* static int undo_stream_raw_read(UndoStream *stream, ase_uint8 *buf, int size); */
/* static void undo_stream_raw_write(UndoStream *stream, ase_uint8 *buf, int size); */

/* General undo routines */

Undo *undo_new(Sprite *sprite)
{
  Undo *undo = (Undo *)gfxobj_new(GFXOBJ_UNDO, sizeof(Undo));
  if (!undo)
    return NULL;

  undo->sprite = sprite;
  undo->undo_stream = undo_stream_new();
  undo->redo_stream = undo_stream_new();
  undo->diff_count = 0;
  undo->diff_saved = 0;
  undo->enabled = TRUE;
  undo->size_limit = 1024*1024;

  return undo;
}

void undo_free(Undo *undo)
{
  undo_stream_free(undo->undo_stream);
  undo_stream_free(undo->redo_stream);

  jfree(undo);
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

      /*     { int c; */
      /*       for (c=0; c<ABS (level); c++) */
      /* 	fprintf (stderr, "  "); */
      /*       fprintf (stderr, "%s: %s\n", */
      /* 	       (state == DO_UNDO) ? "Undo": "Redo", */
      /* 	       undo_actions[chunk->type].name); } */

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
      chunk = link->data;
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

  /* more diff */
  undo->diff_count++;

  /* reset the "redo" stream */
  if (!jlist_empty(undo->redo_stream->chunks)) {
    undo_stream_free(undo->redo_stream);
    undo->redo_stream = undo_stream_new();
  }

  /* "undo" is too big? */
  while (groups > 1 && undo->undo_stream->size > undo->size_limit) {
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
  chunk_open(undo->undo_stream);
  update_undo(undo);
}

static void chunk_open(UndoStream *stream)
{
  UndoChunk *chunk = undo_chunk_new(UNDO_TYPE_OPEN);

  undo_stream_push_chunk(stream, chunk);
}

static void chunk_open_invert(UndoStream *stream, UndoChunk *chunk, int state)
{
  chunk_close(stream);
}

/***********************************************************************

  "close"

     no data

***********************************************************************/

void undo_close(Undo *undo)
{
  chunk_close(undo->undo_stream);
  update_undo(undo);
}

static void chunk_close(UndoStream *stream)
{
  UndoChunk *chunk = undo_chunk_new(UNDO_TYPE_CLOSE);

  undo_stream_push_chunk(stream, chunk);
}

static void chunk_close_invert(UndoStream *stream, UndoChunk *chunk, int state)
{
  chunk_open(stream);
}

/***********************************************************************

  "data"

     DWORD		object ID
     DWORD		data address offset
     DWORD		data size
     BYTE[]		data bytes

***********************************************************************/

void undo_data(Undo *undo, GfxObj *gfxobj, void *data, int size)
{
  chunk_data(undo->undo_stream, gfxobj, data, size);
  update_undo(undo);
}

static void chunk_data(UndoStream *stream, GfxObj *gfxobj, void *data, int size)
{
  UndoChunk *chunk = undo_chunk_new(UNDO_TYPE_DATA);
  unsigned int offset = (unsigned int)(((ase_uint8 *)data) -
				       ((ase_uint8 *)gfxobj));
  unsigned int c;

  undo_chunk_put32(chunk, gfxobj->id);
  undo_chunk_put32(chunk, offset);
  undo_chunk_put32(chunk, size);
  for (c=0; c<size; c++)
    undo_chunk_put8(chunk, ((ase_uint8 *)data)[c]);

  undo_stream_push_chunk(stream, chunk);
}

static void chunk_data_invert(UndoStream *stream, UndoChunk *chunk, int state)
{
  unsigned int id = undo_chunk_get32(chunk);
  unsigned int offset = undo_chunk_get32(chunk);
  unsigned int c, size = undo_chunk_get32(chunk);
  GfxObj *gfxobj = gfxobj_find(id);

  if (gfxobj) {
    void *data = (void *)(((ase_uint8 *)gfxobj) + offset);

    chunk_data(stream, gfxobj, data, size);

    /* get the string from the chunk */
    for (c=0; c<size; c++)
      ((ase_uint8 *)data)[c] = undo_chunk_get8(chunk);
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

void undo_image(Undo *undo, Image *image, int x, int y, int w, int h)
{
  chunk_image(undo->undo_stream, image, x, y, w, h);
  update_undo(undo);
}

static void chunk_image(UndoStream *stream, Image *image, int x, int y, int w, int h)
{
  UndoChunk *chunk = undo_chunk_new(UNDO_TYPE_IMAGE);
  int v, size;

/*   if (x < 0) { */
/*     w += x; */
/*     x = 0; */
/*   } */

/*   if (y < 0) { */
/*     h += y; */
/*     y = 0; */
/*   } */

/*   if (x+w-1 > image->w-1) */
/*     w = image->w-x; */

/*   if (y+h-1 > image->h-1) */
/*     h = image->h-y; */

  size = IMAGE_LINE_SIZE(image, w);

  undo_chunk_put32(chunk, image->gfxobj.id);
  undo_chunk_put8(chunk, image->imgtype);
  undo_chunk_put16(chunk, x);
  undo_chunk_put16(chunk, y);
  undo_chunk_put16(chunk, w);
  undo_chunk_put16(chunk, h);

  for (v=0; v<h; v++)
    undo_chunk_write(chunk, IMAGE_ADDRESS(image, x, y+v), size);

  undo_stream_push_chunk(stream, chunk);
}

static void chunk_image_invert(UndoStream *stream, UndoChunk *chunk, int state)
{
  unsigned long id = undo_chunk_get32(chunk);
  int imgtype = undo_chunk_get8(chunk);
  Image *image = (Image *)gfxobj_find(id);

  if ((image) && (image->gfxobj.type == GFXOBJ_IMAGE) &&
      (image->imgtype == imgtype)) {
    int x, y, w, h;
    int v, size;

    x = undo_chunk_get16(chunk);
    y = undo_chunk_get16(chunk);
    w = undo_chunk_get16(chunk);
    h = undo_chunk_get16(chunk);

    chunk_image(stream, image, x, y, w, h);

    size = IMAGE_LINE_SIZE(image, w);

    for (v=0; v<h; v++)
      undo_chunk_read(chunk, IMAGE_ADDRESS(image, x, y+v), size);
  }
}

/***********************************************************************

  "flip"

     DWORD		image ID
     BYTE		image type
     WORD[4]		x1, y1, x2, y2
     BYTE		1=horizontal 0=vertical

***********************************************************************/

void undo_flip(Undo *undo, Image *image, int x1, int y1, int x2, int y2, int horz)
{
  chunk_flip(undo->undo_stream, image, x1, y1, x2, y2, horz);
  update_undo(undo);
}

static void chunk_flip(UndoStream *stream, Image *image, int x1, int y1, int x2, int y2, int horz)
{
  UndoChunk *chunk = undo_chunk_new(UNDO_TYPE_FLIP);

  undo_chunk_put32(chunk, image->gfxobj.id);
  undo_chunk_put8(chunk, image->imgtype);
  undo_chunk_put16(chunk, x1);
  undo_chunk_put16(chunk, y1);
  undo_chunk_put16(chunk, x2);
  undo_chunk_put16(chunk, y2);
  undo_chunk_put8(chunk, horz);

  undo_stream_push_chunk(stream, chunk);
}

static void chunk_flip_invert(UndoStream *stream, UndoChunk *chunk, int state)
{
  unsigned long id = undo_chunk_get32(chunk);
  int imgtype = undo_chunk_get8(chunk);
  Image *image = (Image *)gfxobj_find(id);

  if ((image) && (image->gfxobj.type == GFXOBJ_IMAGE) &&
      (image->imgtype == imgtype)) {
    int x1, y1, x2, y2;
    int x, y, horz;
    Image *area;

    x1 = undo_chunk_get16(chunk);
    y1 = undo_chunk_get16(chunk);
    x2 = undo_chunk_get16(chunk);
    y2 = undo_chunk_get16(chunk);
    horz = undo_chunk_get8(chunk);

    chunk_flip(stream, image, x1, y1, x2, y2, horz);

    area = image_crop(image, x1, y1, x2-x1+1, y2-y1+1);
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

     DWORD		image ID
     BYTE		image type
     WORD[4]		x1, y1, x2, y2
     WORD		rows
     for each row
       WORD		y
       WORD		columns
       for each column
         WORD[2]	x, w
	 for each pixel ("w" times)
	   BYTE[4]	for RGB images, or
	   BYTE[2]	for Grayscale images, or
	   BYTE		for Indexed images

***********************************************************************/

void undo_dirty(Undo *undo, Dirty *dirty)
{
  chunk_dirty(undo->undo_stream, dirty);
  update_undo(undo);
}

static void chunk_dirty(UndoStream *stream, Dirty *dirty)
{
  UndoChunk *chunk = undo_chunk_new(UNDO_TYPE_DIRTY);
  int u, v;

  undo_chunk_put32(chunk, dirty->image->gfxobj.id);
  undo_chunk_put8(chunk, dirty->image->imgtype);
  undo_chunk_put16(chunk, dirty->x1);
  undo_chunk_put16(chunk, dirty->y1);
  undo_chunk_put16(chunk, dirty->x2);
  undo_chunk_put16(chunk, dirty->y2);

  undo_chunk_put16(chunk, dirty->rows);
  for (v=0; v<dirty->rows; v++) {
    undo_chunk_put16(chunk, dirty->row[v].y);
    undo_chunk_put16(chunk, dirty->row[v].cols);
    for (u=0; u<dirty->row[v].cols; u++) {
      undo_chunk_put16(chunk, dirty->row[v].col[u].x);
      undo_chunk_put16(chunk, dirty->row[v].col[u].w);
      undo_chunk_write(chunk, dirty->row[v].col[u].data,
		       dirty->row[v].col[u].w << IMAGE_SHIFT(dirty->image));
    }
  }

  undo_stream_push_chunk(stream, chunk);
}

static void chunk_dirty_invert(UndoStream *stream, UndoChunk *chunk, int state)
{
  unsigned long id = undo_chunk_get32(chunk);
  int imgtype = undo_chunk_get8 (chunk);
  Image *image = (Image *)gfxobj_find(id);

  if ((image) && (image->gfxobj.type == GFXOBJ_IMAGE) &&
      (image->imgtype == imgtype)) {
    int x1, y1, x2, y2, size;
    int u, v, x, y;
    Dirty *dirty;

    x1 = undo_chunk_get16(chunk);
    y1 = undo_chunk_get16(chunk);
    x2 = undo_chunk_get16(chunk);
    y2 = undo_chunk_get16(chunk);

    dirty = dirty_new(image, x1, y1, x2, y2, FALSE);
    dirty->rows = undo_chunk_get16(chunk);
    dirty->row = jmalloc(sizeof(struct DirtyRow) * dirty->rows);

    for (v=0; v<dirty->rows; v++) {
      dirty->row[v].y = y = undo_chunk_get16(chunk);
      dirty->row[v].cols = undo_chunk_get16(chunk);
      dirty->row[v].col = jmalloc(sizeof(struct DirtyCol) * dirty->row[v].cols);

      for (u=0; u<dirty->row[v].cols; u++) {
 	dirty->row[v].col[u].x = x = undo_chunk_get16(chunk);
	dirty->row[v].col[u].w = undo_chunk_get16(chunk);

	size = dirty->row[v].col[u].w << IMAGE_SHIFT(dirty->image);

	dirty->row[v].col[u].flags = DIRTY_VALID_COLUMN;
	dirty->row[v].col[u].data = jmalloc (size);
	dirty->row[v].col[u].ptr = IMAGE_ADDRESS(dirty->image, x, y);

	undo_chunk_read(chunk, dirty->row[v].col[u].data, size);
      }
    }

    dirty_swap(dirty);
    chunk_dirty(stream, dirty);
    dirty_free(dirty);
  }
}

/***********************************************************************

  "add_image"

     DWORD		stock ID
     DWORD		index

***********************************************************************/

void undo_add_image(Undo *undo, Stock *stock, Image *image)
{
  chunk_add_image(undo->undo_stream, stock, image);
  update_undo(undo);
}

static void chunk_add_image(UndoStream *stream, Stock *stock, Image *image)
{
  UndoChunk *chunk = undo_chunk_new(UNDO_TYPE_ADD_IMAGE);
  int index;

  for (index=0; index<stock->nimage; index++)
    if (stock->image[index] == image)
      break;

  undo_chunk_put32(chunk, stock->gfxobj.id);
  undo_chunk_put32(chunk, index);

  undo_stream_push_chunk(stream, chunk);
}

static void chunk_add_image_invert(UndoStream *stream, UndoChunk *chunk, int state)
{
  unsigned long stock_id = undo_chunk_get32(chunk);
  unsigned long index = undo_chunk_get32(chunk);
  Stock *stock = (Stock *)gfxobj_find(stock_id);

  if (stock) {
    Image *image = stock_get_image(stock, index);
    if (image) {
      chunk_remove_image(stream, stock, image);
      stock_remove_image(stock, image);
      image_free(image);
    }
  }
}

/***********************************************************************

  "remove_image"

     DWORD		stock ID
     DWORD		index
     IMAGE_DATA		see undo_chunk_read/write_image

***********************************************************************/

void undo_remove_image(Undo *undo, Stock *stock, Image *image)
{
  chunk_remove_image(undo->undo_stream, stock, image);
  update_undo(undo);
}

static void chunk_remove_image(UndoStream *stream, Stock *stock, Image *image)
{
  UndoChunk *chunk = undo_chunk_new(UNDO_TYPE_REMOVE_IMAGE);
  int index;

  for (index=0; index<stock->nimage; index++)
    if (stock->image[index] == image)
      break;

  undo_chunk_put32(chunk, stock->gfxobj.id);
  undo_chunk_put32(chunk, index);
  undo_chunk_write_image(chunk, image);

  undo_stream_push_chunk(stream, chunk);
}

static void chunk_remove_image_invert(UndoStream *stream, UndoChunk *chunk, int state)
{
  unsigned long stock_id = undo_chunk_get32(chunk);
  unsigned long index = undo_chunk_get32(chunk);
  Stock *stock = (Stock *)gfxobj_find(stock_id);

  if (stock) {
    Image *image = undo_chunk_read_image(chunk);

    /* ji_assert(image); */

    stock_replace_image(stock, index, image);
    chunk_add_image(stream, stock, image);
  }
}

/***********************************************************************

  "replace_image"

     DWORD		stock ID
     DWORD		index
     IMAGE_DATA		see undo_chunk_read/write_image

***********************************************************************/

void undo_replace_image(Undo *undo, Stock *stock, int index)
{
  chunk_replace_image(undo->undo_stream, stock, index);
  update_undo(undo);
}

static void chunk_replace_image(UndoStream *stream, Stock *stock, int index)
{
  UndoChunk *chunk = undo_chunk_new(UNDO_TYPE_REPLACE_IMAGE);

  undo_chunk_put32(chunk, stock->gfxobj.id);
  undo_chunk_put32(chunk, index);
  undo_chunk_write_image(chunk, stock->image[index]);

  undo_stream_push_chunk(stream, chunk);
}

static void chunk_replace_image_invert(UndoStream *stream, UndoChunk *chunk, int state)
{
  unsigned long stock_id = undo_chunk_get32(chunk);
  unsigned long index = undo_chunk_get32(chunk);
  Stock *stock = (Stock *)gfxobj_find(stock_id);

  if (stock) {
    Image *image = undo_chunk_read_image(chunk);

    /* ji_assert(image); */

    chunk_replace_image(stream, stock, index);

    if (stock->image[index])
      image_free(stock->image[index]);

    stock_replace_image(stock, index, image);
  }
}

/***********************************************************************

  "add_cel"

     DWORD		layer ID
     DWORD		cel ID

***********************************************************************/

void undo_add_cel(Undo *undo, Layer *layer, Cel *cel)
{
  chunk_add_cel(undo->undo_stream, layer, cel);
  update_undo(undo);
}

static void chunk_add_cel(UndoStream *stream, Layer *layer, Cel *cel)
{
  UndoChunk *chunk = undo_chunk_new(UNDO_TYPE_ADD_CEL);

  undo_chunk_put32(chunk, layer->gfxobj.id);
  undo_chunk_put32(chunk, cel->gfxobj.id);

  undo_stream_push_chunk(stream, chunk);
}

static void chunk_add_cel_invert(UndoStream *stream, UndoChunk *chunk, int state)
{
  unsigned long layer_id = undo_chunk_get32(chunk);
  unsigned long cel_id = undo_chunk_get32(chunk);
  Layer *layer = (Layer *)gfxobj_find(layer_id);
  Cel *cel = (Cel *)gfxobj_find(cel_id);

  if (layer && cel) {
    chunk_remove_cel(stream, layer, cel);
    layer_remove_cel(layer, cel);
    cel_free(cel);
  }
}

/***********************************************************************

  "remove_cel"

     DWORD		layer ID
     CEL_DATA		see undo_chunk_read/write_cel

***********************************************************************/

void undo_remove_cel(Undo *undo, Layer *layer, Cel *cel)
{
  chunk_remove_cel(undo->undo_stream, layer, cel);
  update_undo(undo);
}

static void chunk_remove_cel(UndoStream *stream, Layer *layer, Cel *cel)
{
  UndoChunk *chunk = undo_chunk_new(UNDO_TYPE_REMOVE_CEL);

  undo_chunk_put32(chunk, layer->gfxobj.id);
  undo_chunk_write_cel(chunk, cel);

  undo_stream_push_chunk(stream, chunk);
}

static void chunk_remove_cel_invert(UndoStream *stream, UndoChunk *chunk, int state)
{
  unsigned long layer_id = undo_chunk_get32(chunk);
  Layer *layer = (Layer *)gfxobj_find(layer_id);

  if (layer) {
    Cel *cel = undo_chunk_read_cel(chunk);

    /* ji_assert (cel); */

    chunk_add_cel(stream, layer, cel);
    layer_add_cel(layer, cel);
  }
}

/***********************************************************************

  "add_layer"

     DWORD		parent layer set ID
     DWORD		layer ID

***********************************************************************/

void undo_add_layer(Undo *undo, Layer *set, Layer *layer)
{
  chunk_add_layer(undo->undo_stream, set, layer);
  update_undo(undo);
}

static void chunk_add_layer(UndoStream *stream, Layer *set, Layer *layer)
{
  UndoChunk *chunk = undo_chunk_new(UNDO_TYPE_ADD_LAYER);

  undo_chunk_put32(chunk, set->gfxobj.id);
  undo_chunk_put32(chunk, layer->gfxobj.id);

  undo_stream_push_chunk(stream, chunk);
}

static void chunk_add_layer_invert(UndoStream *stream, UndoChunk *chunk, int state)
{
  unsigned long set_id = undo_chunk_get32(chunk);
  unsigned long layer_id = undo_chunk_get32(chunk);
  Layer *set = (Layer *)gfxobj_find(set_id);
  Layer *layer = (Layer *)gfxobj_find(layer_id);

  if (set && layer) {
    chunk_remove_layer(stream, layer);
    layer_remove_layer(set, layer);
    layer_free(layer);
  }
}

/***********************************************************************

  "remove_layer"

     DWORD		parent layer set ID
     DWORD		after layer ID
     LAYER_DATA		see undo_chunk_read/write_layer

***********************************************************************/

void undo_remove_layer(Undo *undo, Layer *layer)
{
  chunk_remove_layer(undo->undo_stream, layer);
  update_undo(undo);
}

static void chunk_remove_layer(UndoStream *stream, Layer *layer)
{
  UndoChunk *chunk = undo_chunk_new(UNDO_TYPE_REMOVE_LAYER);
  Layer *set = (Layer *)layer->parent;
  Layer *after = layer_get_prev(layer);

  undo_chunk_put32(chunk, set->gfxobj.id);
  undo_chunk_put32(chunk, after ? after->gfxobj.id: 0);
  undo_chunk_write_layer(chunk, layer);

  undo_stream_push_chunk(stream, chunk);
}

static void chunk_remove_layer_invert(UndoStream *stream, UndoChunk *chunk, int state)
{
  unsigned long set_id = undo_chunk_get32(chunk);
  unsigned long after_id = undo_chunk_get32(chunk);
  Layer *set = (Layer *)gfxobj_find(set_id);
  Layer *after = (Layer *)gfxobj_find(after_id);

  if (set) {
    Layer *layer = undo_chunk_read_layer(chunk);

    /* ji_assert(layer); */

    chunk_add_layer(stream, set, layer);
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

void undo_move_layer(Undo *undo, Layer *layer)
{
  chunk_move_layer(undo->undo_stream, layer);
  update_undo(undo);
}

static void chunk_move_layer(UndoStream *stream, Layer *layer)
{
  UndoChunk *chunk = undo_chunk_new(UNDO_TYPE_MOVE_LAYER);
  Layer *set = (Layer *)layer->parent;
  Layer *after = layer_get_prev(layer);

  undo_chunk_put32(chunk, set->gfxobj.id);
  undo_chunk_put32(chunk, layer->gfxobj.id);
  undo_chunk_put32(chunk, after ? after->gfxobj.id: 0);

  undo_stream_push_chunk(stream, chunk);
}

static void chunk_move_layer_invert(UndoStream *stream, UndoChunk *chunk, int state)
{
  unsigned long set_id = undo_chunk_get32(chunk);
  unsigned long layer_id = undo_chunk_get32(chunk);
  unsigned long after_id = undo_chunk_get32(chunk);
  Layer *set = (Layer *)gfxobj_find(set_id);
  Layer *layer = (Layer *)gfxobj_find(layer_id);
  Layer *after = (Layer *)gfxobj_find(after_id);

  if (set && layer) {
    chunk_move_layer(stream, layer);
    layer_move_layer(set, layer, after);
  }
}

/***********************************************************************

  "set_layer"

     DWORD		sprite ID
     DWORD		layer ID

***********************************************************************/

void undo_set_layer(Undo *undo, Sprite *sprite)
{
  chunk_set_layer(undo->undo_stream, sprite);
  update_undo(undo);
}

static void chunk_set_layer(UndoStream *stream, Sprite *sprite)
{
  UndoChunk *chunk = undo_chunk_new(UNDO_TYPE_SET_LAYER);

  undo_chunk_put32(chunk, sprite->gfxobj.id);
  undo_chunk_put32(chunk, sprite->layer ? sprite->layer->gfxobj.id: 0);

  undo_stream_push_chunk(stream, chunk);
}

static void chunk_set_layer_invert(UndoStream *stream, UndoChunk *chunk, int state)
{
  unsigned long sprite_id = undo_chunk_get32(chunk);
  unsigned long layer_id = undo_chunk_get32(chunk);
  Sprite *sprite = (Sprite *)gfxobj_find(sprite_id);
  Layer *layer = (Layer *)gfxobj_find(layer_id);

  if (sprite) {
    chunk_set_layer(stream, sprite);

    sprite->layer = layer;
  }
}

/***********************************************************************

  "set_mask"

     DWORD		sprite ID
     MASK_DATA		see undo_chunk_read/write_mask

***********************************************************************/

void undo_set_mask(Undo *undo, Sprite *sprite)
{
  chunk_set_mask(undo->undo_stream, sprite);
  update_undo(undo);
}

static void chunk_set_mask(UndoStream *stream, Sprite *sprite)
{
  UndoChunk *chunk = undo_chunk_new(UNDO_TYPE_SET_MASK);

  undo_chunk_put32(chunk, sprite->gfxobj.id);
  undo_chunk_write_mask(chunk, sprite->mask);

  undo_stream_push_chunk(stream, chunk);
}

static void chunk_set_mask_invert(UndoStream *stream, UndoChunk *chunk, int state)
{
  unsigned long sprite_id = undo_chunk_get32(chunk);
  Sprite *sprite = (Sprite *)gfxobj_find(sprite_id);

  if (sprite) {
    Mask *mask = undo_chunk_read_mask(chunk);

    /* ji_assert(mask); */

    chunk_set_mask(stream, sprite);
    mask_copy(sprite->mask, mask);

    /* change the sprite mask directly */
/*     mask_free(sprite->mask); */
/*     sprite->mask = mask; */
  }
}

/***********************************************************************

  "set_frames"

     DWORD		sprite ID
     DWORD		frames

***********************************************************************/

void undo_set_frames(Undo *undo, Sprite *sprite)
{
  chunk_set_frames(undo->undo_stream, sprite);
  update_undo(undo);
}

static void chunk_set_frames(UndoStream *stream, Sprite *sprite)
{
  UndoChunk *chunk = undo_chunk_new(UNDO_TYPE_SET_FRAMES);

  undo_chunk_put32(chunk, sprite->gfxobj.id);
  undo_chunk_put32(chunk, sprite->frames);

  undo_stream_push_chunk(stream, chunk);
}

static void chunk_set_frames_invert(UndoStream *stream, UndoChunk *chunk, int state)
{
  unsigned long sprite_id = undo_chunk_get32(chunk);
  Sprite *sprite = (Sprite *)gfxobj_find(sprite_id);

  if (sprite) {
    int frames = undo_chunk_get32(chunk);

    chunk_set_frames(stream, sprite);
    sprite_set_frames(sprite, frames);
  }
}

/***********************************************************************

  Helper routines for UndoChunk

***********************************************************************/

static UndoChunk *undo_chunk_new(int type)
{
  UndoChunk *chunk;

  chunk = jnew(UndoChunk, 1);
  if (!chunk)
    return NULL;

  chunk->type = type;
  chunk->max_size = 0;
  chunk->size = 0;
  chunk->pos = 0;
  chunk->data = NULL;

  return chunk;
}

static void undo_chunk_free(UndoChunk *chunk)
{
  if (chunk->data)
    jfree(chunk->data);

  jfree(chunk);
}

static int undo_chunk_get8(UndoChunk *chunk)
{
  if (chunk->pos < chunk->size)
    return chunk->data[chunk->pos++];
  else
    return EOF;
}

static void undo_chunk_put8(UndoChunk *chunk, int c)
{
  if (chunk->size == chunk->max_size) {
    chunk->max_size += 4096;
    chunk->data = jrealloc(chunk->data, chunk->max_size);
  }
  chunk->data[chunk->size++] = c;
}

static int undo_chunk_get16(UndoChunk *chunk)
{
  int b1, b2;

  if ((b1 = undo_chunk_get8(chunk)) != EOF)
    if ((b2 = undo_chunk_get8(chunk)) != EOF)
      return ((b2 << 8) | b1);

  return EOF;
}

static void undo_chunk_put16(UndoChunk *chunk, int c)
{
  int b1, b2;

  b1 = (c & 0xFF00) >> 8;
  b2 = c & 0x00FF;

  undo_chunk_put8(chunk, b2);
  undo_chunk_put8(chunk, b1);
}

static long undo_chunk_get32(UndoChunk *chunk)
{
  int b1, b2, b3, b4;

  if ((b1 = undo_chunk_get8(chunk)) != EOF)
    if ((b2 = undo_chunk_get8(chunk)) != EOF)
      if ((b3 = undo_chunk_get8(chunk)) != EOF)
	if ((b4 = undo_chunk_get8(chunk)) != EOF)
	  return (((long)b4 << 24) | ((long)b3 << 16) |
		  ((long)b2 << 8) | (long)b1);

  return EOF;
}

static void undo_chunk_put32(UndoChunk *chunk, long c)
{
  int b1, b2, b3, b4;

  b1 = (int)((c & 0xFF000000L) >> 24);
  b2 = (int)((c & 0x00FF0000L) >> 16);
  b3 = (int)((c & 0x0000FF00L) >> 8);
  b4 = (int)c & 0x00FF;

  undo_chunk_put8(chunk, b4);
  undo_chunk_put8(chunk, b3);
  undo_chunk_put8(chunk, b2);
  undo_chunk_put8(chunk, b1);
}

/* static double undo_chunk_get64(UndoChunk *chunk) */
/* { */
/*   unsigned long longs[2]; */
/*   double value; */

/*   longs[0] = undo_chunk_get32(chunk); */
/*   longs[1] = undo_chunk_get32(chunk); */

/*   memcpy(&value, longs, sizeof (longs)); */

/*   return value; */
/* } */

/* static void undo_chunk_put64(UndoChunk *chunk, double c) */
/* { */
/*   unsigned long longs[2]; */

/*   memcpy(longs, &c, sizeof (longs)); */

/*   undo_chunk_put32(chunk, longs[0]); */
/*   undo_chunk_put32(chunk, longs[1]); */
/* } */

static int undo_chunk_read(UndoChunk *chunk, ase_uint8 *buf, int size)
{
  if (chunk->pos+size > chunk->size)
    return 0;

  memcpy(buf, chunk->data+chunk->pos, size);
  chunk->pos += size;

  return size;
}

static void undo_chunk_write(UndoChunk *chunk, const ase_uint8 *buf, int size)
{
  if (chunk->size+size > chunk->max_size) {
    chunk->max_size = chunk->size+size;
    chunk->data = jrealloc(chunk->data, chunk->max_size);
  }
  memcpy(chunk->data+chunk->size, buf, size);
  chunk->size += size;
}

static char *undo_chunk_read_string(UndoChunk *chunk)
{
  unsigned int count = undo_chunk_get16(chunk);
  char *string;

  if (count > 0) {
    string = jmalloc(count+1);
    undo_chunk_read(chunk, string, count);
    string[count] = 0;
  }
  else
    string = NULL;

  return string;
}

static void undo_chunk_write_string(UndoChunk *chunk, const char *string)
{
  int count = string ? strlen(string): 0;

  undo_chunk_put16(chunk, count);
  if (string)
    undo_chunk_write(chunk, string, count);
}

static Cel *undo_chunk_read_cel(UndoChunk *chunk)
{
  unsigned long cel_id = undo_chunk_get32(chunk);
  int frpos = undo_chunk_get16(chunk);
  int image = undo_chunk_get16(chunk);
  int x = (short)undo_chunk_get16(chunk);
  int y = (short)undo_chunk_get16(chunk);
  int opacity = undo_chunk_get8(chunk);
  Cel *cel = cel_new(frpos, image);
  cel_set_position(cel, x, y);
  cel_set_opacity(cel, opacity);

  _gfxobj_set_id((GfxObj *)cel, cel_id);
  return cel;
}

static void undo_chunk_write_cel(UndoChunk *chunk, Cel *cel)
{
  undo_chunk_put32(chunk, cel->gfxobj.id);
  undo_chunk_put16(chunk, cel->frame);
  undo_chunk_put16(chunk, cel->image);
  undo_chunk_put16(chunk, cel->x);
  undo_chunk_put16(chunk, cel->y);
  undo_chunk_put8(chunk, cel->opacity);
}

static Layer *undo_chunk_read_layer(UndoChunk *chunk)
{
  unsigned long layer_id = undo_chunk_get32(chunk);
  char *name = undo_chunk_read_string(chunk); /* name */
  int flags = undo_chunk_get8(chunk);	      /* properties */
  int type = undo_chunk_get32(chunk);	      /* type */
  int sprite_id = undo_chunk_get32(chunk);    /* sprite */
  Layer *layer = NULL;
  Sprite *sprite = (Sprite *)gfxobj_find(sprite_id);

  switch (type) {

    case GFXOBJ_LAYER_IMAGE: {
      int blend_mode = undo_chunk_get8(chunk); /* blend mode */
      int c, cels = undo_chunk_get16(chunk); /* how many cels */

      /* create layer */
      layer = layer_new(sprite);

      /* set blend mode */
      layer_set_blend_mode(layer, blend_mode);

      /* read cels */
      for (c=0; c<cels; c++)
	layer_add_cel(layer, undo_chunk_read_cel(chunk));
      break;
    }

    case GFXOBJ_LAYER_SET: {
      int c, count = undo_chunk_get16(chunk);
      Layer *child;

      layer = layer_set_new(sprite);

      for (c=0; c<count; c++) {
	child = undo_chunk_read_layer(chunk);
	if (child)
	  layer_add_layer(layer, child);
      }
      break;
    }

  }

  if (layer) {
    if (name)
      layer_set_name(layer, name);

    if (flags & 1)
      layer->readable = TRUE;

    if (flags & 2)
      layer->writable = TRUE;

    _gfxobj_set_id((GfxObj *)layer, layer_id);
  }

  if (name)
    jfree(name);

  return layer;
}

static void undo_chunk_write_layer(UndoChunk *chunk, Layer *layer)
{
  JLink link;

  undo_chunk_put32(chunk, layer->gfxobj.id);		  /* ID */
  undo_chunk_write_string(chunk, layer->name);		  /* name */
  undo_chunk_put8(chunk, (((layer->readable)?1:0) |
			  (((layer->writable)?1:0)<<1))); /* properties */
  undo_chunk_put32(chunk, layer->gfxobj.type);		  /* type */
  undo_chunk_put32(chunk, layer->sprite->gfxobj.id);	  /* sprite */

  switch (layer->gfxobj.type) {

    case GFXOBJ_LAYER_IMAGE:
      /* properties */
      undo_chunk_put8(chunk, layer->blend_mode); /* blend mode */
      /* cels */
      undo_chunk_put16(chunk, jlist_length(layer->cels));
      JI_LIST_FOR_EACH(layer->cels, link)
	undo_chunk_write_cel(chunk, link->data);
      break;

    case GFXOBJ_LAYER_SET:
      /* how many sub-layers */
      undo_chunk_put16(chunk, jlist_length(layer->layers));

      JI_LIST_FOR_EACH(layer->layers, link)
	undo_chunk_write_layer(chunk, link->data);
      break;

  }
}

static Image *undo_chunk_read_image(UndoChunk *chunk)
{
  unsigned long image_id;
  int imgtype;
  int width;
  int height;
  Image *image;
  int c, size;

  image_id = undo_chunk_get32(chunk); /* ID */
  if (!image_id)
    return NULL;

  imgtype = undo_chunk_get8(chunk); /* imgtype */
  width = undo_chunk_get16(chunk); /* width */
  height = undo_chunk_get16(chunk); /* height */

  image = image_new(imgtype, width, height);
  size = IMAGE_LINE_SIZE(image, image->w);

  for (c=0; c<image->h; c++)
    undo_chunk_read(chunk, image->line[c], size);

  _gfxobj_set_id((GfxObj *)image, image_id);
  return image;
}

static void undo_chunk_write_image(UndoChunk *chunk, Image *image)
{
  int c, size;

  undo_chunk_put32(chunk, image ? image->gfxobj.id: 0); /* ID */
  if (!image)
    return;

  size = IMAGE_LINE_SIZE(image, image->w);

  undo_chunk_put8(chunk, image->imgtype); /* imgtype */
  undo_chunk_put16(chunk, image->w); /* width */
  undo_chunk_put16(chunk, image->h); /* height */

  for (c=0; c<image->h; c++)
    undo_chunk_write(chunk, image->line[c], size);
}

static Mask *undo_chunk_read_mask(UndoChunk *chunk)
{
/*   unsigned long mask_id = undo_chunk_get32(chunk); /\* ID *\/ */
  int x = (short)undo_chunk_get16(chunk); /* xpos */
  int y = (short)undo_chunk_get16(chunk); /* ypos */
  int width = undo_chunk_get16(chunk); /* width */
  int height = undo_chunk_get16(chunk); /* height */
  int c, size = (width+7)/8;
  Mask *mask;

  mask = mask_new();
  if (!mask)
    return NULL;

  if (width > 0 && height > 0) {
    mask_union(mask, x, y, width, height);

    for (c=0; c<mask->h; c++)
      undo_chunk_read(chunk, mask->bitmap->line[c], size);
  }

/*   _gfxobj_set_id((GfxObj *)mask, mask_id); */
  return mask;
}

static void undo_chunk_write_mask(UndoChunk *chunk, Mask *mask)
{
  int c, size = (mask->w+7)/8;

/*   undo_chunk_put32 (chunk, mask->gfxobj.id); /\* ID *\/ */
  undo_chunk_put16(chunk, mask->x); /* xpos */
  undo_chunk_put16(chunk, mask->y); /* ypos */
  undo_chunk_put16(chunk, mask->bitmap ? mask->w: 0); /* width */
  undo_chunk_put16(chunk, mask->bitmap ? mask->h: 0); /* height */

  if (mask->bitmap)
    for (c=0; c<mask->h; c++)
      undo_chunk_write(chunk, mask->bitmap->line[c], size);
}

/***********************************************************************

  Helper routines for UndoStream (a serie of chunks)

***********************************************************************/

static UndoStream *undo_stream_new(void)
{
  UndoStream *stream;

  stream = jnew(UndoStream, 1);
  if (!stream)
    return NULL;

  stream->chunks = jlist_new();
  stream->size = 0;

  return stream;
}

static void undo_stream_free(UndoStream *stream)
{
  JLink link;

  JI_LIST_FOR_EACH(stream->chunks, link)
    undo_chunk_free(link->data);

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

    chunk = link->data;
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
