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

#include <vector>
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
#include "raster/palette.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "raster/undo.h"

/* undo state */
enum {
  DO_UNDO,
  DO_REDO,
};

// undo types
enum {
  // group
  UNDO_TYPE_OPEN,
  UNDO_TYPE_CLOSE,

  // data management
  UNDO_TYPE_DATA,

  // image management
  UNDO_TYPE_IMAGE,
  UNDO_TYPE_FLIP,
  UNDO_TYPE_DIRTY,

  // stock management
  UNDO_TYPE_ADD_IMAGE,
  UNDO_TYPE_REMOVE_IMAGE,
  UNDO_TYPE_REPLACE_IMAGE,

  // cel management
  UNDO_TYPE_ADD_CEL,
  UNDO_TYPE_REMOVE_CEL,

  // layer management
  UNDO_TYPE_SET_LAYER_NAME,
  UNDO_TYPE_ADD_LAYER,
  UNDO_TYPE_REMOVE_LAYER,
  UNDO_TYPE_MOVE_LAYER,
  UNDO_TYPE_SET_LAYER,

  // palette management
  UNDO_TYPE_ADD_PALETTE,
  UNDO_TYPE_REMOVE_PALETTE,
  UNDO_TYPE_REMAP_PALETTE,

  /* misc */
  UNDO_TYPE_SET_MASK,
  UNDO_TYPE_SET_IMGTYPE,
  UNDO_TYPE_SET_SIZE,
  UNDO_TYPE_SET_FRAME,
  UNDO_TYPE_SET_FRAMES,
  UNDO_TYPE_SET_FRLEN,
};

struct UndoChunkData;
struct UndoChunkImage;
struct UndoChunkFlip;
struct UndoChunkDirty;
struct UndoChunkAddImage;
struct UndoChunkRemoveImage;
struct UndoChunkReplaceImage;
struct UndoChunkAddCel;
struct UndoChunkRemoveCel;
struct UndoChunkSetLayerName;
struct UndoChunkAddLayer;
struct UndoChunkRemoveLayer;
struct UndoChunkMoveLayer;
struct UndoChunkSetLayer;
struct UndoChunkAddPalette;
struct UndoChunkRemovePalette;
struct UndoChunkSetMask;
struct UndoChunkSetImgType;
struct UndoChunkSetSize;
struct UndoChunkSetFrame;
struct UndoChunkSetFrames;
struct UndoChunkSetFrlen;
struct UndoChunkRemapPalette;

struct UndoChunk
{
  int type;
  int size;
  const char *label;
};

struct UndoStream
{
  Undo* undo;
  JList chunks;
  int size;
};

struct UndoAction
{
  const char *name;
  void (*invert)(UndoStream* stream, UndoChunk* chunk, int state);
};

static void run_undo(Undo* undo, int state);
static void discard_undo_tail(Undo* undo);
static int count_undo_groups(UndoStream* undo_stream);
static bool out_of_group(UndoStream* undo_stream);
static void update_undo(Undo* undo);

/* Undo actions */

static void chunk_open_new(UndoStream* stream);
static void chunk_open_invert(UndoStream* stream, UndoChunk* chunk, int state);

static void chunk_close_new(UndoStream* stream);
static void chunk_close_invert(UndoStream* stream, UndoChunk* chunk, int state);

static void chunk_data_new(UndoStream* stream, GfxObj *gfxobj, void *data, int size);
static void chunk_data_invert(UndoStream* stream, UndoChunkData *chunk, int state);

static void chunk_image_new(UndoStream* stream, Image* image, int x, int y, int w, int h);
static void chunk_image_invert(UndoStream* stream, UndoChunkImage* chunk, int state);

static void chunk_flip_new(UndoStream* stream, Image* image, int x1, int y1, int x2, int y2, bool horz);
static void chunk_flip_invert(UndoStream* stream, UndoChunkFlip *chunk, int state);

static void chunk_dirty_new(UndoStream* stream, Dirty *dirty);
static void chunk_dirty_invert(UndoStream* stream, UndoChunkDirty *chunk, int state);

static void chunk_add_image_new(UndoStream* stream, Stock *stock, int image_index);
static void chunk_add_image_invert(UndoStream* stream, UndoChunkAddImage* chunk, int state);

static void chunk_remove_image_new(UndoStream* stream, Stock *stock, int image_index);
static void chunk_remove_image_invert(UndoStream* stream, UndoChunkRemoveImage* chunk, int state);

static void chunk_replace_image_new(UndoStream* stream, Stock *stock, int image_index);
static void chunk_replace_image_invert(UndoStream* stream, UndoChunkReplaceImage* chunk, int state);

static void chunk_add_cel_new(UndoStream* stream, Layer* layer, Cel* cel);
static void chunk_add_cel_invert(UndoStream* stream, UndoChunkAddCel* chunk, int state);

static void chunk_remove_cel_new(UndoStream* stream, Layer* layer, Cel* cel);
static void chunk_remove_cel_invert(UndoStream* stream, UndoChunkRemoveCel* chunk, int state);

static void chunk_set_layer_name_new(UndoStream* stream, Layer *layer);
static void chunk_set_layer_name_invert(UndoStream* stream, UndoChunkSetLayerName* chunk, int state);

static void chunk_add_layer_new(UndoStream* stream, Layer* set, Layer* layer);
static void chunk_add_layer_invert(UndoStream* stream, UndoChunkAddLayer* chunk, int state);

static void chunk_remove_layer_new(UndoStream* stream, Layer* layer);
static void chunk_remove_layer_invert(UndoStream* stream, UndoChunkRemoveLayer* chunk, int state);

static void chunk_move_layer_new(UndoStream* stream, Layer* layer);
static void chunk_move_layer_invert(UndoStream* stream, UndoChunkMoveLayer* chunk, int state);

static void chunk_set_layer_new(UndoStream* stream, Sprite *sprite);
static void chunk_set_layer_invert(UndoStream* stream, UndoChunkSetLayer* chunk, int state);

static void chunk_add_palette_new(UndoStream* stream, Sprite *sprite, Palette* palette);
static void chunk_add_palette_invert(UndoStream* stream, UndoChunkAddPalette *chunk, int state);

static void chunk_remove_palette_new(UndoStream* stream, Sprite *sprite, Palette* palette);
static void chunk_remove_palette_invert(UndoStream* stream, UndoChunkRemovePalette *chunk, int state);

static void chunk_remap_palette_new(UndoStream* stream, Sprite* sprite, int frame_from, int frame_to, const std::vector<int>& mapping);
static void chunk_remap_palette_invert(UndoStream* stream, UndoChunkRemapPalette *chunk, int state);

static void chunk_set_mask_new(UndoStream* stream, Sprite *sprite);
static void chunk_set_mask_invert(UndoStream* stream, UndoChunkSetMask* chunk, int state);

static void chunk_set_imgtype_new(UndoStream* stream, Sprite *sprite);
static void chunk_set_imgtype_invert(UndoStream* stream, UndoChunkSetImgType *chunk, int state);

static void chunk_set_size_new(UndoStream* stream, Sprite *sprite);
static void chunk_set_size_invert(UndoStream* stream, UndoChunkSetSize *chunk, int state);

static void chunk_set_frame_new(UndoStream* stream, Sprite *sprite);
static void chunk_set_frame_invert(UndoStream* stream, UndoChunkSetFrame *chunk, int state);

static void chunk_set_frames_new(UndoStream* stream, Sprite *sprite);
static void chunk_set_frames_invert(UndoStream* stream, UndoChunkSetFrames *chunk, int state);

static void chunk_set_frlen_new(UndoStream* stream, Sprite *sprite, int frame);
static void chunk_set_frlen_invert(UndoStream* stream, UndoChunkSetFrlen *chunk, int state);

#define DECL_UNDO_ACTION(name) \
  { #name, (void (*)(UndoStream* , UndoChunk* , int))chunk_##name##_invert }

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
  DECL_UNDO_ACTION(set_layer_name),
  DECL_UNDO_ACTION(add_layer),
  DECL_UNDO_ACTION(remove_layer),
  DECL_UNDO_ACTION(move_layer),
  DECL_UNDO_ACTION(set_layer),
  DECL_UNDO_ACTION(add_palette),
  DECL_UNDO_ACTION(remove_palette),
  DECL_UNDO_ACTION(remap_palette),
  DECL_UNDO_ACTION(set_mask),
  DECL_UNDO_ACTION(set_imgtype),
  DECL_UNDO_ACTION(set_size),
  DECL_UNDO_ACTION(set_frame),
  DECL_UNDO_ACTION(set_frames),
  DECL_UNDO_ACTION(set_frlen),
};

/* UndoChunk */

static UndoChunk* undo_chunk_new(UndoStream* stream, int type, int size);
static void undo_chunk_free(UndoChunk* chunk);

/* Raw data */

static Dirty *read_raw_dirty(ase_uint8* raw_data);
static ase_uint8* write_raw_dirty(ase_uint8* raw_data, Dirty *dirty);
static int get_raw_dirty_size(Dirty *dirty);

static Image* read_raw_image(ase_uint8* raw_data);
static ase_uint8* write_raw_image(ase_uint8* raw_data, Image* image);
static int get_raw_image_size(Image* image);

static Cel* read_raw_cel(ase_uint8* raw_data);
static ase_uint8* write_raw_cel(ase_uint8* raw_data, Cel* cel);
static int get_raw_cel_size(Cel* cel);

static Layer* read_raw_layer(ase_uint8* raw_data);
static ase_uint8* write_raw_layer(ase_uint8* raw_data, Layer* layer);
static int get_raw_layer_size(Layer* layer);

static Palette* read_raw_palette(ase_uint8* raw_data);
static ase_uint8* write_raw_palette(ase_uint8* raw_data, Palette* palette);
static int get_raw_palette_size(Palette* palette);

static Mask* read_raw_mask(ase_uint8* raw_data);
static ase_uint8* write_raw_mask(ase_uint8* raw_data, Mask* mask);
static int get_raw_mask_size(Mask* mask);

/* UndoStream */

static UndoStream* undo_stream_new(Undo* undo);
static void undo_stream_free(UndoStream* stream);

static UndoChunk* undo_stream_pop_chunk(UndoStream* stream, bool tail);
static void undo_stream_push_chunk(UndoStream* stream, UndoChunk* chunk);

//////////////////////////////////////////////////////////////////////

Undo::Undo(Sprite* sprite)
  : GfxObj(GFXOBJ_UNDO)
{
  this->sprite = sprite;
  this->undo_stream = undo_stream_new(this); // TODO try/catch
  this->redo_stream = undo_stream_new(this);
  this->diff_count = 0;
  this->diff_saved = 0;
  this->enabled = true;
  this->label = NULL;
}

Undo::~Undo()
{
  undo_stream_free(this->undo_stream);
  undo_stream_free(this->redo_stream);
}

//////////////////////////////////////////////////////////////////////
/* General undo routines */

Undo* undo_new(Sprite *sprite)
{
  return new Undo(sprite);
}

void undo_free(Undo* undo)
{
  ASSERT(undo);
  delete undo;
}

int undo_get_memsize(const Undo* undo)
{
  ASSERT(undo);
  return
    undo->undo_stream->size +
    undo->redo_stream->size;
}

void undo_enable(Undo* undo)
{
  ASSERT(undo);
  undo->enabled = true;
}

void undo_disable(Undo* undo)
{
  ASSERT(undo);
  undo->enabled = false;
}

bool undo_is_enabled(const Undo* undo)
{
  ASSERT(undo);
  return undo->enabled ? true: false;
}

bool undo_is_disabled(const Undo* undo)
{
  ASSERT(undo);
  return !undo_is_enabled(undo);
}

bool undo_can_undo(const Undo* undo)
{
  ASSERT(undo);
  return !jlist_empty(undo->undo_stream->chunks);
}

bool undo_can_redo(const Undo* undo)
{
  ASSERT(undo);
  return !jlist_empty(undo->redo_stream->chunks);
}

void undo_do_undo(Undo* undo)
{
  ASSERT(undo);
  run_undo(undo, DO_UNDO);
}

void undo_do_redo(Undo* undo)
{
  ASSERT(undo);
  run_undo(undo, DO_REDO);
}

void undo_clear_redo(Undo* undo)
{
  ASSERT(undo);
  if (!jlist_empty(undo->redo_stream->chunks)) {
    undo_stream_free(undo->redo_stream);
    undo->redo_stream = undo_stream_new(undo);
  }
}

void undo_set_label(Undo* undo, const char *label)
{
  undo->label = label;
}

const char *undo_get_next_undo_label(const Undo* undo)
{
  UndoChunk* chunk;

  ASSERT(undo_can_undo(undo));

  chunk = reinterpret_cast<UndoChunk*>(jlist_first_data(undo->undo_stream->chunks));
  return chunk->label;
}

const char *undo_get_next_redo_label(const Undo* undo)
{
  UndoChunk* chunk;

  ASSERT(undo_can_redo(undo));

  chunk = reinterpret_cast<UndoChunk*>(jlist_first_data(undo->redo_stream->chunks));
  return chunk->label;
}

static void run_undo(Undo* undo, int state)
{
  UndoStream* undo_stream = ((state == DO_UNDO)? undo->undo_stream:
						 undo->redo_stream);
  UndoStream* redo_stream = ((state == DO_REDO)? undo->undo_stream:
						 undo->redo_stream);
  UndoChunk* chunk;
  int level = 0;

  do {
    chunk = undo_stream_pop_chunk(undo_stream, false); // read from head
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

static void discard_undo_tail(Undo* undo)
{
  UndoStream* undo_stream = undo->undo_stream;
  UndoChunk* chunk;
  int level = 0;

  do {
    chunk = undo_stream_pop_chunk(undo_stream, true); // read from tail
    if (!chunk)
      break;

    if (chunk->type == UNDO_TYPE_OPEN)
      level++;
    else if (chunk->type == UNDO_TYPE_CLOSE)
      level--;

    undo_chunk_free(chunk);
  } while (level);
}

static int count_undo_groups(UndoStream* undo_stream)
{
  UndoChunk* chunk;
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

static bool out_of_group(UndoStream* undo_stream)
{
  UndoChunk* chunk;
  int level = 0;
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
  }

  return level == 0;
}

/* called every time a new undo is added */
static void update_undo(Undo* undo)
{
  int undo_size_limit = get_config_int("Options", "UndoSizeLimit", 8)*1024*1024;

  /* more diff */
  undo->diff_count++;

  /* reset the "redo" stream */
  undo_clear_redo(undo);

  if (out_of_group(undo->undo_stream)) {
    int groups = count_undo_groups(undo->undo_stream);

    /* "undo" is too big? */
    while (groups > 1 && undo->undo_stream->size > undo_size_limit) {
      discard_undo_tail(undo);
      groups--;
    }
  }
}

/***********************************************************************

  "open"

     no data

***********************************************************************/

void undo_open(Undo* undo)
{
  chunk_open_new(undo->undo_stream);
  update_undo(undo);
}

static void chunk_open_new(UndoStream* stream)
{
  undo_chunk_new(stream,
		 UNDO_TYPE_OPEN,
		 sizeof(UndoChunk));
}

static void chunk_open_invert(UndoStream* stream, UndoChunk* chunk, int state)
{
  chunk_close_new(stream);
}

/***********************************************************************

  "close"

     no data

***********************************************************************/

void undo_close(Undo* undo)
{
  chunk_close_new(undo->undo_stream);
  update_undo(undo);
}

static void chunk_close_new(UndoStream* stream)
{
  undo_chunk_new(stream,
		 UNDO_TYPE_CLOSE,
		 sizeof(UndoChunk));
}

static void chunk_close_invert(UndoStream* stream, UndoChunk* chunk, int state)
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

void undo_data(Undo* undo, GfxObj *gfxobj, void *data, int size)
{
  chunk_data_new(undo->undo_stream, gfxobj, data, size);
  update_undo(undo);
}

static void chunk_data_new(UndoStream* stream, GfxObj *gfxobj, void *data, int size)
{
  UndoChunkData *chunk;
  ase_uint32 offset = (unsigned int)(((ase_uint8* )data) -
				     ((ase_uint8* )gfxobj));

  ASSERT(size >= 1);

  chunk = (UndoChunkData *)
    undo_chunk_new(stream,
		   UNDO_TYPE_DATA,
		   sizeof(UndoChunkData)+size);

  chunk->gfxobj_id = gfxobj->id;
  chunk->dataoffset = offset;
  chunk->datasize = size;

  memcpy(chunk->data, data, size);
}

static void chunk_data_invert(UndoStream* stream, UndoChunkData *chunk, int state)
{
  unsigned int id = chunk->gfxobj_id;
  unsigned int offset = chunk->dataoffset;
  unsigned int size = chunk->datasize;
  GfxObj *gfxobj = gfxobj_find(id);

  if (gfxobj) {
    void *data = (void *)(((ase_uint8* )gfxobj) + offset);

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

void undo_image(Undo* undo, Image* image, int x, int y, int w, int h)
{
  chunk_image_new(undo->undo_stream, image, x, y, w, h);
  update_undo(undo);
}

static void chunk_image_new(UndoStream* stream, Image* image, int x, int y, int w, int h)
{
  UndoChunkImage* chunk;
  ase_uint8* ptr;
  int v, size;

  ASSERT(w >= 1 && h >= 1);
  ASSERT(x >= 0 && y >= 0 && x+w <= image->w && y+h <= image->h);
  
  size = image_line_size(image, w);

  chunk = (UndoChunkImage* )
    undo_chunk_new(stream,
		   UNDO_TYPE_IMAGE,
		   sizeof(UndoChunkImage) + size*h);
  
  chunk->image_id = image->id;
  chunk->imgtype = image->imgtype;
  chunk->x = x;
  chunk->y = y;
  chunk->w = w;
  chunk->h = h;

  ptr = chunk->data;
  for (v=0; v<h; ++v) {
    memcpy(ptr, image_address(image, x, y+v), size);
    ptr += size;
  }
}

static void chunk_image_invert(UndoStream* stream, UndoChunkImage* chunk, int state)
{
  unsigned int id = chunk->image_id;
  int imgtype = chunk->imgtype;
  Image* image = (Image*)gfxobj_find(id);

  if ((image) && (image->type == GFXOBJ_IMAGE) &&
      (image->imgtype == imgtype)) {
    int x, y, w, h;
    ase_uint8* ptr;
    int v, size;

    x = chunk->x;
    y = chunk->y;
    w = chunk->w;
    h = chunk->h;

    /* backup the current image portion */
    chunk_image_new(stream, image, x, y, w, h);

    /* restore the old image portion */
    size = image_line_size(image, chunk->w);
    ptr = chunk->data;

    for (v=0; v<h; ++v) {
      memcpy(image_address(image, x, y+v), ptr, size);
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

void undo_flip(Undo* undo, Image* image, int x1, int y1, int x2, int y2, bool horz)
{
  chunk_flip_new(undo->undo_stream, image, x1, y1, x2, y2, horz);
  update_undo(undo);
}

static void chunk_flip_new(UndoStream* stream, Image* image, int x1, int y1, int x2, int y2, bool horz)
{
  UndoChunkFlip *chunk = (UndoChunkFlip *)
    undo_chunk_new(stream,
		   UNDO_TYPE_FLIP,
		   sizeof(UndoChunkFlip));

  chunk->image_id = image->id;
  chunk->imgtype = image->imgtype;
  chunk->x1 = x1;
  chunk->y1 = y1;
  chunk->x2 = x2;
  chunk->y2 = y2;
  chunk->horz = horz ? 1: 0;
}

static void chunk_flip_invert(UndoStream* stream, UndoChunkFlip* chunk, int state)
{
  Image* image = (Image*)gfxobj_find(chunk->image_id);

  if ((image) &&
      (image->type == GFXOBJ_IMAGE) &&
      (image->imgtype == chunk->imgtype)) {
    int x1 = chunk->x1;
    int y1 = chunk->y1;
    int x2 = chunk->x2;
    int y2 = chunk->y2;
    bool horz = (chunk->horz != 0);

    chunk_flip_new(stream, image, x1, y1, x2, y2, horz);

    Image* area = image_crop(image, x1, y1, x2-x1+1, y2-y1+1, 0);
    int x, y;

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

void undo_dirty(Undo* undo, Dirty *dirty)
{
  chunk_dirty_new(undo->undo_stream, dirty);
  update_undo(undo);
}

static void chunk_dirty_new(UndoStream* stream, Dirty *dirty)
{
  UndoChunkDirty *chunk = (UndoChunkDirty *)
    undo_chunk_new(stream,
		   UNDO_TYPE_DIRTY,
		   sizeof(UndoChunkDirty)+get_raw_dirty_size(dirty));

  write_raw_dirty(chunk->data, dirty);
}

static void chunk_dirty_invert(UndoStream* stream, UndoChunkDirty *chunk, int state)
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

void undo_add_image(Undo* undo, Stock *stock, int image_index)
{
  chunk_add_image_new(undo->undo_stream, stock, image_index);
  update_undo(undo);
}

static void chunk_add_image_new(UndoStream* stream, Stock *stock, int image_index)
{
  UndoChunkAddImage* chunk = (UndoChunkAddImage* )
    undo_chunk_new(stream,
		   UNDO_TYPE_ADD_IMAGE,
		   sizeof(UndoChunkAddImage));

  chunk->stock_id = stock->id;
  chunk->image_index = image_index;
}

static void chunk_add_image_invert(UndoStream* stream, UndoChunkAddImage* chunk, int state)
{
  unsigned int stock_id = chunk->stock_id;
  unsigned int image_index = chunk->image_index;
  Stock *stock = (Stock *)gfxobj_find(stock_id);

  if (stock) {
    Image* image = stock_get_image(stock, image_index);
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

void undo_remove_image(Undo* undo, Stock *stock, int image_index)
{
  chunk_remove_image_new(undo->undo_stream, stock, image_index);
  update_undo(undo);
}

static void chunk_remove_image_new(UndoStream* stream, Stock *stock, int image_index)
{
  Image* image = stock->image[image_index];
  UndoChunkRemoveImage* chunk = (UndoChunkRemoveImage* )
    undo_chunk_new(stream,
		   UNDO_TYPE_REMOVE_IMAGE,
		   sizeof(UndoChunkRemoveImage)+get_raw_image_size(image));

  chunk->stock_id = stock->id;
  chunk->image_index = image_index;

  write_raw_image(chunk->data, image);
}

static void chunk_remove_image_invert(UndoStream* stream, UndoChunkRemoveImage* chunk, int state)
{
  unsigned int stock_id = chunk->stock_id;
  unsigned int image_index = chunk->image_index;
  Stock *stock = (Stock *)gfxobj_find(stock_id);

  if (stock) {
    Image* image = read_raw_image(chunk->data);

    /* ASSERT(image != NULL); */

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

void undo_replace_image(Undo* undo, Stock *stock, int image_index)
{
  chunk_replace_image_new(undo->undo_stream, stock, image_index);
  update_undo(undo);
}

static void chunk_replace_image_new(UndoStream* stream, Stock *stock, int image_index)
{
  Image* image = stock_get_image(stock, image_index);
  UndoChunkReplaceImage* chunk = (UndoChunkReplaceImage* )
    undo_chunk_new(stream,
		   UNDO_TYPE_REPLACE_IMAGE,
		   sizeof(UndoChunkReplaceImage)+get_raw_image_size(image));

  chunk->stock_id = stock->id;
  chunk->image_index = image_index;

  write_raw_image(chunk->data, image);
}

static void chunk_replace_image_invert(UndoStream* stream, UndoChunkReplaceImage* chunk, int state)
{
  unsigned long stock_id = chunk->stock_id;
  unsigned long image_index = chunk->image_index;
  Stock* stock = (Stock*)gfxobj_find(stock_id);

  if (stock) {
    // read the image to be restored from the chunk
    Image* image = read_raw_image(chunk->data);

    // save the current image in the (redo) stream
    chunk_replace_image_new(stream, stock, image_index);
    Image* old_image = stock_get_image(stock, image_index);

    // replace the image in the stock
    stock_replace_image(stock, image_index, image);

    // destroy the old image
    image_free(old_image);
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

void undo_add_cel(Undo* undo, Layer* layer, Cel* cel)
{
  chunk_add_cel_new(undo->undo_stream, layer, cel);
  update_undo(undo);
}

static void chunk_add_cel_new(UndoStream* stream, Layer* layer, Cel* cel)
{
  UndoChunkAddCel* chunk = (UndoChunkAddCel* )
    undo_chunk_new(stream,
		   UNDO_TYPE_ADD_CEL,
		   sizeof(UndoChunkAddCel));

  chunk->layer_id = layer->id;
  chunk->cel_id = cel->id;
}

static void chunk_add_cel_invert(UndoStream* stream, UndoChunkAddCel* chunk, int state)
{
  LayerImage* layer = (LayerImage*)gfxobj_find(chunk->layer_id);
  Cel* cel = (Cel*)gfxobj_find(chunk->cel_id);

  if (layer && cel) {
    chunk_remove_cel_new(stream, layer, cel);

    layer->remove_cel(cel);
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

void undo_remove_cel(Undo* undo, Layer* layer, Cel* cel)
{
  chunk_remove_cel_new(undo->undo_stream, layer, cel);
  update_undo(undo);
}

static void chunk_remove_cel_new(UndoStream* stream, Layer* layer, Cel* cel)
{
  UndoChunkRemoveCel* chunk = (UndoChunkRemoveCel*)
    undo_chunk_new(stream,
		   UNDO_TYPE_REMOVE_CEL,
		   sizeof(UndoChunkRemoveCel)+get_raw_cel_size(cel));

  chunk->layer_id = layer->id;
  write_raw_cel(chunk->data, cel);
}

static void chunk_remove_cel_invert(UndoStream* stream, UndoChunkRemoveCel* chunk, int state)
{
  unsigned int layer_id = chunk->layer_id;
  LayerImage* layer = (LayerImage*)gfxobj_find(layer_id);

  if (layer) {
    Cel* cel = read_raw_cel(chunk->data);

    /* ASSERT(cel != NULL); */

    chunk_add_cel_new(stream, layer, cel);
    layer->add_cel(cel);
  }
}

/***********************************************************************

  "set_layer_name"

     DWORD		layer ID
     DWORD		name length
     BYTES[length]	name text

***********************************************************************/

struct UndoChunkSetLayerName
{
  UndoChunk head;
  ase_uint32 layer_id;
  ase_uint16 name_length;
  ase_uint8 name_text[0];
};

void undo_set_layer_name(Undo* undo, Layer* layer)
{
  chunk_set_layer_name_new(undo->undo_stream, layer);
  update_undo(undo);
}

static void chunk_set_layer_name_new(UndoStream* stream, Layer *layer)
{
  std::string layer_name = layer->get_name();

  UndoChunkSetLayerName* chunk = (UndoChunkSetLayerName*)
    undo_chunk_new(stream,
  		   UNDO_TYPE_SET_LAYER_NAME,
  		   sizeof(UndoChunkSetLayerName) + layer_name.size());

  chunk->layer_id = layer->id;
  chunk->name_length = layer_name.size();

  for (int c=0; c<chunk->name_length; c++)
    chunk->name_text[c] = layer_name[c];
}

static void chunk_set_layer_name_invert(UndoStream* stream, UndoChunkSetLayerName* chunk, int state)
{
  Layer* layer = (Layer*)gfxobj_find(chunk->layer_id);

  if (layer) {
    chunk_set_layer_name_new(stream, layer);

    std::string layer_name;
    layer_name.reserve(chunk->name_length);

    for (int c=0; c<chunk->name_length; c++)
      layer_name.push_back(chunk->name_text[c]);

    layer->set_name(layer_name.c_str());
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
  ase_uint32 folder_id;
  ase_uint32 layer_id;
};

void undo_add_layer(Undo* undo, Layer* folder, Layer* layer)
{
  chunk_add_layer_new(undo->undo_stream, folder, layer);
  update_undo(undo);
}

static void chunk_add_layer_new(UndoStream* stream, Layer* folder, Layer* layer)
{
  UndoChunkAddLayer* chunk = (UndoChunkAddLayer* )
    undo_chunk_new(stream,
		   UNDO_TYPE_ADD_LAYER,
		   sizeof(UndoChunkAddLayer));

  chunk->folder_id = folder->id;
  chunk->layer_id = layer->id;
}

static void chunk_add_layer_invert(UndoStream* stream, UndoChunkAddLayer* chunk, int state)
{
  LayerFolder* folder = (LayerFolder*)gfxobj_find(chunk->folder_id);
  Layer* layer = (Layer*)gfxobj_find(chunk->layer_id);

  if (folder && layer) {
    chunk_remove_layer_new(stream, layer);

    folder->remove_layer(layer);
    delete layer;
  }
}

/***********************************************************************

  "remove_layer"

     DWORD		parent layer folder ID
     DWORD		after layer ID
     LAYER_DATA		see read/write_raw_layer

***********************************************************************/

struct UndoChunkRemoveLayer
{
  UndoChunk head;
  ase_uint32 folder_id;
  ase_uint32 after_id;
  ase_uint8 data[0];
};

void undo_remove_layer(Undo* undo, Layer* layer)
{
  chunk_remove_layer_new(undo->undo_stream, layer);
  update_undo(undo);
}

static void chunk_remove_layer_new(UndoStream* stream, Layer* layer)
{
  UndoChunkRemoveLayer* chunk = (UndoChunkRemoveLayer*)
    undo_chunk_new(stream,
		   UNDO_TYPE_REMOVE_LAYER,
		   sizeof(UndoChunkRemoveLayer)+get_raw_layer_size(layer));
  LayerFolder* folder = layer->get_parent();
  Layer* after = layer->get_prev();

  chunk->folder_id = folder->id;
  chunk->after_id = after != NULL ? after->id: 0;

  write_raw_layer(chunk->data, layer);
}

static void chunk_remove_layer_invert(UndoStream* stream, UndoChunkRemoveLayer* chunk, int state)
{
  LayerFolder* folder = (LayerFolder*)gfxobj_find(chunk->folder_id);
  Layer* after = (Layer*)gfxobj_find(chunk->after_id);

  if (folder) {
    Layer* layer = read_raw_layer(chunk->data);

    /* ASSERT(layer != NULL); */

    chunk_add_layer_new(stream, folder, layer);
    folder->add_layer(layer);
    folder->move_layer(layer, after);
  }
}

/***********************************************************************

  "move_layer"

     DWORD		parent layer folder ID
     DWORD		layer ID
     DWORD		after layer ID

***********************************************************************/

struct UndoChunkMoveLayer
{
  UndoChunk head;
  ase_uint32 folder_id;
  ase_uint32 layer_id;
  ase_uint32 after_id;
};

void undo_move_layer(Undo* undo, Layer* layer)
{
  chunk_move_layer_new(undo->undo_stream, layer);
  update_undo(undo);
}

static void chunk_move_layer_new(UndoStream* stream, Layer* layer)
{
  UndoChunkMoveLayer* chunk = (UndoChunkMoveLayer* )
    undo_chunk_new(stream,
		   UNDO_TYPE_MOVE_LAYER,
		   sizeof(UndoChunkMoveLayer));
  LayerFolder* folder = layer->get_parent();
  Layer* after = layer->get_prev();

  chunk->folder_id = folder->id;
  chunk->layer_id = layer->id;
  chunk->after_id = after ? after->id: 0;
}

static void chunk_move_layer_invert(UndoStream* stream, UndoChunkMoveLayer* chunk, int state)
{
  LayerFolder* folder = (LayerFolder*)gfxobj_find(chunk->folder_id);
  Layer* layer = (Layer*)gfxobj_find(chunk->layer_id);
  Layer* after = (Layer*)gfxobj_find(chunk->after_id);

  if (folder == NULL || layer == NULL)
    throw undo_exception("chunk_move_layer_invert");

  chunk_move_layer_new(stream, layer);
  folder->move_layer(layer, after);
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

void undo_set_layer(Undo* undo, Sprite *sprite)
{
  chunk_set_layer_new(undo->undo_stream, sprite);
  update_undo(undo);
}

static void chunk_set_layer_new(UndoStream* stream, Sprite *sprite)
{
  UndoChunkSetLayer* chunk = (UndoChunkSetLayer*)
    undo_chunk_new(stream,
		   UNDO_TYPE_SET_LAYER,
		   sizeof(UndoChunkSetLayer));

  chunk->sprite_id = sprite->id;
  chunk->layer_id = sprite->getCurrentLayer() ? sprite->getCurrentLayer()->id: 0;
}

static void chunk_set_layer_invert(UndoStream* stream, UndoChunkSetLayer* chunk, int state)
{
  Sprite *sprite = (Sprite *)gfxobj_find(chunk->sprite_id);
  Layer* layer = (Layer* )gfxobj_find(chunk->layer_id);

  if (sprite == NULL)
    throw undo_exception("chunk_set_layer_invert");

  chunk_set_layer_new(stream, sprite);

  sprite->setCurrentLayer(layer);
}

/***********************************************************************

  "add_palette"

     DWORD		sprite ID
     DWORD		palette ID

***********************************************************************/

struct UndoChunkAddPalette
{
  UndoChunk head;
  ase_uint32 sprite_id;
  ase_uint32 palette_id;
};

void undo_add_palette(Undo* undo, Sprite *sprite, Palette* palette)
{
  chunk_add_palette_new(undo->undo_stream, sprite, palette);
  update_undo(undo);
}

static void chunk_add_palette_new(UndoStream* stream, Sprite *sprite, Palette* palette)
{
  UndoChunkAddPalette* chunk = (UndoChunkAddPalette*)
    undo_chunk_new(stream,
		   UNDO_TYPE_ADD_PALETTE,
		   sizeof(UndoChunkAddPalette));

  chunk->sprite_id = sprite->id;
  chunk->palette_id = palette->id;
}

static void chunk_add_palette_invert(UndoStream* stream, UndoChunkAddPalette *chunk, int state)
{
  Sprite* sprite = (Sprite*)gfxobj_find(chunk->sprite_id);
  Palette* palette = (Palette*)gfxobj_find(chunk->palette_id);

  if (sprite == NULL || palette == NULL)
    throw undo_exception("chunk_add_palette_invert");

  chunk_remove_palette_new(stream, sprite, palette);
  sprite->deletePalette(palette);
}

/***********************************************************************

  "remove_palette"

     DWORD		sprite ID
     PALETTE_DATA	see read/write_raw_palette

***********************************************************************/

struct UndoChunkRemovePalette
{
  UndoChunk head;
  ase_uint32 sprite_id;
  ase_uint8 data[0];
};

void undo_remove_palette(Undo* undo, Sprite *sprite, Palette* palette)
{
  chunk_remove_palette_new(undo->undo_stream, sprite, palette);
  update_undo(undo);
}

static void chunk_remove_palette_new(UndoStream* stream, Sprite *sprite, Palette* palette)
{
  UndoChunkRemovePalette* chunk = (UndoChunkRemovePalette*)
    undo_chunk_new(stream,
		   UNDO_TYPE_REMOVE_PALETTE,
		   sizeof(UndoChunkRemovePalette)+get_raw_palette_size(palette));

  chunk->sprite_id = sprite->id;
  write_raw_palette(chunk->data, palette);
}

static void chunk_remove_palette_invert(UndoStream* stream, UndoChunkRemovePalette *chunk, int state)
{
  Sprite *sprite = (Sprite *)gfxobj_find(chunk->sprite_id);
  if (sprite == NULL)
    throw undo_exception("chunk_remove_palette_invert");

  Palette* palette = read_raw_palette(chunk->data);

  chunk_add_palette_new(stream, sprite, palette);
  sprite->setPalette(palette, true);

  delete palette;
}

/***********************************************************************

  "remap_palette"

     DWORD		sprite ID
     DWORD		first frame in range
     DWORD		last frame in range
     BYTE[256]		mapping table

***********************************************************************/

struct UndoChunkRemapPalette
{
  UndoChunk head;
  ase_uint32 sprite_id;
  ase_uint32 frame_from;
  ase_uint32 frame_to;
  ase_uint8 mapping[256];
};

void undo_remap_palette(Undo* undo, Sprite* sprite, int frame_from, int frame_to, const std::vector<int>& mapping)
{
  chunk_remap_palette_new(undo->undo_stream, sprite, frame_from, frame_to, mapping);
  update_undo(undo);
}

static void chunk_remap_palette_new(UndoStream* stream, Sprite *sprite, int frame_from, int frame_to, const std::vector<int>& mapping)
{
  UndoChunkRemapPalette* chunk = (UndoChunkRemapPalette*)
    undo_chunk_new(stream,
		   UNDO_TYPE_REMAP_PALETTE,
		   sizeof(UndoChunkRemapPalette));

  chunk->sprite_id = sprite->id;
  chunk->frame_from = frame_from;
  chunk->frame_to = frame_to;

  ASSERT(mapping.size() == 256 && "Mapping tables must have 256 entries");

  for (size_t c=0; c<256; c++)
    chunk->mapping[c] = mapping[c];
}

static void chunk_remap_palette_invert(UndoStream* stream, UndoChunkRemapPalette* chunk, int state)
{
  Sprite *sprite = (Sprite *)gfxobj_find(chunk->sprite_id);
  if (sprite == NULL)
    throw undo_exception("chunk_remap_palette_invert");

  // Inverse mapping
  std::vector<int> inverse_mapping(256);
  for (size_t c=0; c<256; ++c)
    inverse_mapping[chunk->mapping[c]] = c;

  chunk_remap_palette_new(stream, sprite, chunk->frame_from, chunk->frame_to, inverse_mapping);

  // Remap in inverse order
  sprite->remapImages(chunk->frame_from, chunk->frame_to, inverse_mapping);
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

void undo_set_mask(Undo* undo, Sprite *sprite)
{
  chunk_set_mask_new(undo->undo_stream, sprite);
  update_undo(undo);
}

static void chunk_set_mask_new(UndoStream* stream, Sprite *sprite)
{
  UndoChunkSetMask* chunk = (UndoChunkSetMask* )
    undo_chunk_new(stream,
		   UNDO_TYPE_SET_MASK,
		   sizeof(UndoChunkSetMask)+get_raw_mask_size(sprite->getMask()));

  chunk->sprite_id = sprite->id;
  write_raw_mask(chunk->data, sprite->getMask());
}

static void chunk_set_mask_invert(UndoStream* stream, UndoChunkSetMask* chunk, int state)
{
  Sprite *sprite = (Sprite *)gfxobj_find(chunk->sprite_id);

  if (sprite) {
    Mask* mask = read_raw_mask(chunk->data);

    chunk_set_mask_new(stream, sprite);
    mask_copy(sprite->getMask(), mask);

    mask_free(mask);
  }
}

/***********************************************************************

  "set_imgtype"

     DWORD		sprite ID
     DWORD		imgtype

***********************************************************************/

struct UndoChunkSetImgType
{
  UndoChunk head;
  ase_uint32 sprite_id;
  ase_uint32 imgtype;
};

void undo_set_imgtype(Undo* undo, Sprite* sprite)
{
  chunk_set_imgtype_new(undo->undo_stream, sprite);
  update_undo(undo);
}

static void chunk_set_imgtype_new(UndoStream* stream, Sprite* sprite)
{
  UndoChunkSetImgType* chunk = (UndoChunkSetImgType*)
    undo_chunk_new(stream,
		   UNDO_TYPE_SET_IMGTYPE,
		   sizeof(UndoChunkSetImgType));

  chunk->sprite_id = sprite->id;
  chunk->imgtype = sprite->getImgType();
}

static void chunk_set_imgtype_invert(UndoStream* stream, UndoChunkSetImgType *chunk, int state)
{
  Sprite *sprite = (Sprite *)gfxobj_find(chunk->sprite_id);

  if (sprite) {
    chunk_set_imgtype_new(stream, sprite);
    sprite->setImgType(chunk->imgtype);

    // Regenerate extras
    sprite->destroyExtraCel();

  }
}

/***********************************************************************

  "set_size"

     DWORD		sprite ID
     DWORD		width
     DWORD		height

***********************************************************************/

struct UndoChunkSetSize
{
  UndoChunk head;
  ase_uint32 sprite_id;
  ase_uint32 width;
  ase_uint32 height;
};

void undo_set_size(Undo* undo, Sprite* sprite)
{
  chunk_set_size_new(undo->undo_stream, sprite);
  update_undo(undo);
}

static void chunk_set_size_new(UndoStream* stream, Sprite* sprite)
{
  UndoChunkSetSize* chunk = (UndoChunkSetSize*)
    undo_chunk_new(stream,
		   UNDO_TYPE_SET_SIZE,
		   sizeof(UndoChunkSetSize));

  chunk->sprite_id = sprite->id;
  chunk->width = sprite->getWidth();
  chunk->height = sprite->getHeight();
}

static void chunk_set_size_invert(UndoStream* stream, UndoChunkSetSize *chunk, int state)
{
  Sprite *sprite = (Sprite *)gfxobj_find(chunk->sprite_id);

  if (sprite) {
    chunk_set_size_new(stream, sprite);
    sprite->setSize(chunk->width, chunk->height);
  }
}

/***********************************************************************

  "set_frame"

     DWORD		sprite ID
     DWORD		frame

***********************************************************************/

struct UndoChunkSetFrame
{
  UndoChunk head;
  ase_uint32 sprite_id;
  ase_uint32 frame;
};

void undo_set_frame(Undo* undo, Sprite* sprite)
{
  chunk_set_frame_new(undo->undo_stream, sprite);
  update_undo(undo);
}

static void chunk_set_frame_new(UndoStream* stream, Sprite* sprite)
{
  UndoChunkSetFrame* chunk = (UndoChunkSetFrame*)
    undo_chunk_new(stream,
		   UNDO_TYPE_SET_FRAME,
		   sizeof(UndoChunkSetFrame));

  chunk->sprite_id = sprite->id;
  chunk->frame = sprite->getCurrentFrame();
}

static void chunk_set_frame_invert(UndoStream* stream, UndoChunkSetFrame *chunk, int state)
{
  Sprite *sprite = (Sprite *)gfxobj_find(chunk->sprite_id);

  if (sprite) {
    chunk_set_frame_new(stream, sprite);
    sprite->setCurrentFrame(chunk->frame);
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

void undo_set_frames(Undo* undo, Sprite *sprite)
{
  chunk_set_frames_new(undo->undo_stream, sprite);
  update_undo(undo);
}

static void chunk_set_frames_new(UndoStream* stream, Sprite *sprite)
{
  UndoChunkSetFrames *chunk = (UndoChunkSetFrames *)
    undo_chunk_new(stream,
		   UNDO_TYPE_SET_FRAMES,
		   sizeof(UndoChunkSetFrames));

  chunk->sprite_id = sprite->id;
  chunk->frames = sprite->getTotalFrames();
}

static void chunk_set_frames_invert(UndoStream* stream, UndoChunkSetFrames *chunk, int state)
{
  Sprite *sprite = (Sprite *)gfxobj_find(chunk->sprite_id);

  if (sprite) {
    chunk_set_frames_new(stream, sprite);
    sprite->setTotalFrames(chunk->frames);
  }
}

/***********************************************************************

  "set_frlen"

     DWORD		sprite ID
     DWORD		frame
     DWORD		duration

***********************************************************************/

struct UndoChunkSetFrlen
{
  UndoChunk head;
  ase_uint32 sprite_id;
  ase_uint32 frame;
  ase_uint32 duration;
};

void undo_set_frlen(Undo* undo, Sprite *sprite, int frame)
{
  chunk_set_frlen_new(undo->undo_stream, sprite, frame);
  update_undo(undo);
}

static void chunk_set_frlen_new(UndoStream* stream, Sprite *sprite, int frame)
{
  ASSERT(frame >= 0 && frame < sprite->getTotalFrames());

  UndoChunkSetFrlen *chunk = (UndoChunkSetFrlen *)
    undo_chunk_new(stream,
		   UNDO_TYPE_SET_FRLEN,
		   sizeof(UndoChunkSetFrlen));

  chunk->sprite_id = sprite->id;
  chunk->frame = frame;
  chunk->duration = sprite->getFrameDuration(frame);
}

static void chunk_set_frlen_invert(UndoStream* stream, UndoChunkSetFrlen *chunk, int state)
{
  Sprite *sprite = (Sprite *)gfxobj_find(chunk->sprite_id);

  if (sprite != NULL) {
    chunk_set_frlen_new(stream, sprite, chunk->frame);
    sprite->setFrameDuration(chunk->frame, chunk->duration);
  }
}

/***********************************************************************

  Helper routines for UndoChunk

***********************************************************************/

static UndoChunk* undo_chunk_new(UndoStream* stream, int type, int size)
{
  UndoChunk* chunk;

  ASSERT(size >= (int)sizeof(UndoChunk));

  chunk = (UndoChunk* )jmalloc0(size);
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

static void undo_chunk_free(UndoChunk* chunk)
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

static Dirty *read_raw_dirty(ase_uint8* raw_data)
{
  ase_uint32 dword;
  ase_uint16 word;
  int x1, y1, x2, y2, size;
  unsigned int image_id;
  int u, v, x, y;
  int imgtype;
  Image* image;
  Dirty *dirty = NULL;

  read_raw_uint32(image_id);
  read_raw_uint8(imgtype);

  image = (Image* )gfxobj_find(image_id);

  if ((image) &&
      (image->type == GFXOBJ_IMAGE) &&
      (image->imgtype == imgtype)) {

    read_raw_uint16(x1);
    read_raw_uint16(y1);
    read_raw_uint16(x2);
    read_raw_uint16(y2);

    dirty = dirty_new(image, x1, y1, x2, y2, false);
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

	  size = image_line_size(dirty->image, dirty->row[v].col[u].w);

	  dirty->row[v].col[u].flags = DIRTY_VALID_COLUMN;
	  dirty->row[v].col[u].data = jmalloc(size);
	  dirty->row[v].col[u].ptr = image_address(dirty->image, x, y);

	  read_raw_data(dirty->row[v].col[u].data, size);
	}
      }
    }
  }

  return dirty;
}

static ase_uint8* write_raw_dirty(ase_uint8* raw_data, Dirty *dirty)
{
  ase_uint32 dword;
  ase_uint16 word;
  int u, v, size;

  write_raw_uint32(dirty->image->id);
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

      size = image_line_size(dirty->image, dirty->row[v].col[u].w);
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
      size += image_line_size(dirty->image, dirty->row[v].col[u].w);
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

static Image* read_raw_image(ase_uint8* raw_data)
{
  ase_uint32 dword;
  ase_uint16 word;
  gfxobj_id image_id;
  int imgtype;
  int width;
  int height;
  Image* image;
  int c, size;

  read_raw_uint32(image_id);	/* ID */
  if (!image_id)
    return NULL;

  read_raw_uint8(imgtype);	   /* imgtype */
  read_raw_uint16(width);	   /* width */
  read_raw_uint16(height);	   /* height */

  image = image_new(imgtype, width, height);
  size = image_line_size(image, image->w);

  for (c=0; c<image->h; c++)
    read_raw_data(image->line[c], size);

  _gfxobj_set_id((GfxObj *)image, image_id);
  return image;
}

static ase_uint8* write_raw_image(ase_uint8* raw_data, Image* image)
{
  ase_uint32 dword;
  ase_uint16 word;
  int c, size;

  write_raw_uint32(image->id);		   /* ID */
  write_raw_uint8(image->imgtype);	   /* imgtype */
  write_raw_uint16(image->w);		   /* width */
  write_raw_uint16(image->h);		   /* height */

  size = image_line_size(image, image->w);
  for (c=0; c<image->h; c++)
    write_raw_data(image->line[c], size);

  return raw_data;
}

static int get_raw_image_size(Image* image)
{
  ASSERT(image != NULL);
  return 4+1+2+2+image_line_size(image, image->w) * image->h;
}

/***********************************************************************

  Raw cel data

     DWORD		cel ID
     WORD		frame
     WORD		image index
     WORD[2]		x, y
     WORD		opacity

***********************************************************************/

static Cel* read_raw_cel(ase_uint8* raw_data)
{
  ase_uint32 dword;
  ase_uint16 word;
  int frame, image, x, y, opacity;
  gfxobj_id cel_id;
  Cel* cel;

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

static ase_uint8* write_raw_cel(ase_uint8* raw_data, Cel* cel)
{
  ase_uint32 dword;
  ase_uint16 word;

  write_raw_uint32(cel->id);
  write_raw_uint16(cel->frame);
  write_raw_uint16(cel->image);
  write_raw_int16(cel->x);
  write_raw_int16(cel->y);
  write_raw_uint16(cel->opacity);

  return raw_data;
}

static int get_raw_cel_size(Cel* cel)
{
  return 4+2*5;
}

/***********************************************************************

  Raw layer data

***********************************************************************/

static Layer* read_raw_layer(ase_uint8* raw_data)
{
  ase_uint32 dword;
  ase_uint16 word;
  gfxobj_id layer_id, sprite_id;
  std::vector<char> name(1);
  int name_length, flags, layer_type;
  Layer* layer = NULL;
  Sprite *sprite;

  read_raw_uint32(layer_id);			    /* ID */

  read_raw_uint16(name_length);			    /* name length */
  name.resize(name_length+1);
  if (name_length > 0) {
    read_raw_data(&name[0], name_length);	    /* name */
    name[name_length] = 0;
  }
  else
    name[0] = 0;

  read_raw_uint8(flags);			    /* flags */
  read_raw_uint16(layer_type);			    /* type */
  read_raw_uint32(sprite_id);			    /* sprite */

  sprite = (Sprite*)gfxobj_find(sprite_id);

  switch (layer_type) {

    case GFXOBJ_LAYER_IMAGE: {
      int c, blend_mode, cels;

      read_raw_uint8(blend_mode); /* blend mode */
      read_raw_uint16(cels);	  /* cels */

      /* create layer */
      layer = new LayerImage(sprite);

      /* set blend mode */
      static_cast<LayerImage*>(layer)->set_blend_mode(blend_mode);

      /* read cels */
      for (c=0; c<cels; c++) {
	Cel* cel;
	ase_uint8 has_image;

	/* read the cel */
	cel = read_raw_cel(raw_data);
	raw_data += get_raw_cel_size(cel);

	/* add the cel in the layer */
	static_cast<LayerImage*>(layer)->add_cel(cel);

	/* read the image */
	read_raw_uint8(has_image);
	if (has_image != 0) {
	  Image* image = read_raw_image(raw_data);
	  raw_data += get_raw_image_size(image);

	  stock_replace_image(layer->getSprite()->getStock(), cel->image, image);
	}
      }
      break;
    }

    case GFXOBJ_LAYER_FOLDER: {
      int c, layers;

      /* create the layer set */
      layer = new LayerFolder(sprite);

      /* read how many sub-layers */
      read_raw_uint16(layers);

      for (c=0; c<layers; c++) {
	Layer* child = read_raw_layer(raw_data);
	if (child) {
	  static_cast<LayerFolder*>(layer)->add_layer(child);
	  raw_data += get_raw_layer_size(child);
	}
	else
	  break;
      }
      break;
    }
      
  }

  if (layer != NULL) {
    layer->set_name(&name[0]);
    *layer->flags_addr() = flags;

    _gfxobj_set_id((GfxObj*)layer, layer_id);
  }

  return layer;
}

static ase_uint8* write_raw_layer(ase_uint8* raw_data, Layer* layer)
{
  ase_uint32 dword;
  ase_uint16 word;
  std::string name = layer->get_name();

  write_raw_uint32(layer->id);			    /* ID */

  write_raw_uint16(name.size());		    /* name length */
  if (!name.empty())
    write_raw_data(name.c_str(), name.size());	    /* name */

  write_raw_uint8(*layer->flags_addr());	    /* flags */
  write_raw_uint16(layer->type);		    /* type */
  write_raw_uint32(layer->getSprite()->id);	    /* sprite */

  switch (layer->type) {

    case GFXOBJ_LAYER_IMAGE: {
      // blend mode
      write_raw_uint8(static_cast<LayerImage*>(layer)->get_blend_mode());

      // cels
      write_raw_uint16(static_cast<LayerImage*>(layer)->get_cels_count());

      CelIterator it = static_cast<LayerImage*>(layer)->get_cel_begin();
      CelIterator end = static_cast<LayerImage*>(layer)->get_cel_end();

      for (; it != end; ++it) {
	Cel* cel = *it;
	raw_data = write_raw_cel(raw_data, cel);

	Image* image = layer->getSprite()->getStock()->image[cel->image];
	ASSERT(image != NULL);

	write_raw_uint8(1);
	raw_data = write_raw_image(raw_data, image);
      }
      break;
    }

    case GFXOBJ_LAYER_FOLDER: {
      LayerIterator it = static_cast<LayerFolder*>(layer)->get_layer_begin();
      LayerIterator end = static_cast<LayerFolder*>(layer)->get_layer_end();

      // how many sub-layers
      write_raw_uint16(static_cast<LayerFolder*>(layer)->get_layers_count());

      for (; it != end; ++it)
	raw_data = write_raw_layer(raw_data, *it);
      break;
    }

  }

  return raw_data;
}

static int get_raw_layer_size(Layer* layer)
{
  int size = 4+2+layer->get_name().size()+1+2+4;

  switch (layer->type) {

    case GFXOBJ_LAYER_IMAGE: {
      size += 1;		// blend mode
      size += 2;		// num of cels

      CelIterator it = static_cast<LayerImage*>(layer)->get_cel_begin();
      CelIterator end = static_cast<LayerImage*>(layer)->get_cel_end();

      for (; it != end; ++it) {
	Cel* cel = *it;
	size += get_raw_cel_size(cel);
	size++;			// has image?

	Image* image = layer->getSprite()->getStock()->image[cel->image];
	size += get_raw_image_size(image);
      }
      break;
    }

    case GFXOBJ_LAYER_FOLDER: {
      size += 2;		// how many sub-layers

      LayerIterator it = static_cast<LayerFolder*>(layer)->get_layer_begin();
      LayerIterator end = static_cast<LayerFolder*>(layer)->get_layer_end();

      for (; it != end; ++it)
	size += get_raw_layer_size(*it);
      break;
    }

  }

  return size;
}

/***********************************************************************

  Raw palette data

      WORD		frame
      WORD		ncolors
      for each color	("ncolors" times)
        DWORD		_rgba color

***********************************************************************/

static Palette* read_raw_palette(ase_uint8* raw_data)
{
  ase_uint32 dword;
  ase_uint16 word;
  ase_uint32 color;
  int frame, ncolors;
  Palette* palette;

  read_raw_uint16(frame);	/* frame */
  read_raw_uint16(ncolors);	/* ncolors */

  palette = new Palette(frame, ncolors);
  if (!palette)
    return NULL;

  for (int c=0; c<ncolors; c++) {
    read_raw_uint32(color);
    palette->setEntry(c, color);
  }

  return palette;
}

static ase_uint8* write_raw_palette(ase_uint8* raw_data, Palette* palette)
{
  ase_uint32 dword;
  ase_uint16 word;
  ase_uint32 color;

  write_raw_uint16(palette->getFrame()); // frame
  write_raw_uint16(palette->size());	 // number of colors

  for (int c=0; c<palette->size(); c++) {
    color = palette->getEntry(c);
    write_raw_uint32(color);
  }

  return raw_data;
}

static int get_raw_palette_size(Palette* palette)
{
  // 2 WORD + 4 BYTES*ncolors
  return 2*2 + 4*palette->size();
}


/***********************************************************************

  Raw mask data

      WORD[4]		x, y, w, h
      for each line	("h" times)
        for each packet	("((w+7)/8)" times)
          BYTE		8 pixels of the mask

***********************************************************************/

static Mask* read_raw_mask(ase_uint8* raw_data)
{
  ase_uint16 word;
  int x, y, w, h;
  int c, size;
  Mask* mask;

  read_raw_uint16(x);		/* xpos */
  read_raw_uint16(y);		/* ypos */
  read_raw_uint16(w);		/* width */
  read_raw_uint16(h);		/* height */

  mask = mask_new();
  if (!mask)
    return NULL;

  if (w > 0 && h > 0) {
    size = (w+7)/8;

    mask->add(x, y, w, h);
    for (c=0; c<mask->h; c++)
      read_raw_data(mask->bitmap->line[c], size);
  }

  return mask;
}

static ase_uint8* write_raw_mask(ase_uint8* raw_data, Mask* mask)
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

static int get_raw_mask_size(Mask* mask)
{
  int size = (mask->w+7)/8;

  return 2*4 + (mask->bitmap ? mask->h*size: 0);
}

/***********************************************************************

  Helper routines for UndoStream (a serie of UndoChunks)

***********************************************************************/

static UndoStream* undo_stream_new(Undo* undo)
{
  UndoStream* stream;

  stream = jnew(UndoStream, 1);
  if (!stream)
    return NULL;

  stream->undo = undo;
  stream->chunks = jlist_new();
  stream->size = 0;

  return stream;
}

static void undo_stream_free(UndoStream* stream)
{
  JLink link;

  JI_LIST_FOR_EACH(stream->chunks, link)
    undo_chunk_free(reinterpret_cast<UndoChunk*>(link->data));

  jlist_free(stream->chunks);
  jfree(stream);
}

static UndoChunk* undo_stream_pop_chunk(UndoStream* stream, bool tail)
{
  UndoChunk* chunk;
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

static void undo_stream_push_chunk(UndoStream* stream, UndoChunk* chunk)
{
  jlist_prepend(stream->chunks, chunk);
  stream->size += chunk->size;
}
