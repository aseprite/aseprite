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

#include <stdio.h>

#include "jinete/jlist.h"

#include "console/console.h"
#include "file/ase_format.h"
#include "file/file.h"
#include "raster/raster.h"

#endif

#define ASE_FILE_MAGIC			0xA5E0
#define ASE_FILE_FRAME_MAGIC		0xF1FA

#define ASE_FILE_CHUNK_FLI_COLOR	11
#define ASE_FILE_CHUNK_LAYER		0x2004
#define ASE_FILE_CHUNK_CEL		0x2005
#define ASE_FILE_CHUNK_MASK		0x2016
#define ASE_FILE_CHUNK_PATH		0x2017

#define ASE_FILE_RAW_CEL		0
#define ASE_FILE_LINK_CEL		1
#define ASE_FILE_RLE_COMPRESSED_CEL	2

typedef struct ASE_Header
{
  long pos;

  unsigned long size;
  unsigned short magic;
  unsigned short frames;
  unsigned short width;
  unsigned short height;
  unsigned short depth;
  unsigned long flags;
  unsigned short speed;	/* deprecated, use "duration" of FrameHeader */
  unsigned long next;
  unsigned long frit;
  unsigned char bgcolor[4];
} ASE_Header;

typedef struct ASE_FrameHeader
{
  unsigned long size;
  unsigned short magic;
  unsigned short chunks;
  unsigned short duration;
} ASE_FrameHeader;

static Sprite *load_ASE(const char *filename);
static int save_ASE(Sprite *sprite);

static ASE_FrameHeader *current_frame_header = NULL;
static int chunk_type;
static int chunk_start;

static Sprite *ase_file_read(const char *filename);
static int ase_file_write(Sprite *sprite);

static int ase_file_read_header(FILE *f, ASE_Header *header);
static void ase_file_prepare_header(FILE *f, ASE_Header *header, Sprite *sprite);
static void ase_file_write_header(FILE *f, ASE_Header *header);

static void ase_file_read_frame_header(FILE *f, ASE_FrameHeader *frame_header);
static void ase_file_prepare_frame_header(FILE *f, ASE_FrameHeader *frame_header);
static void ase_file_write_frame_header(FILE *f, ASE_FrameHeader *frame_header);

static void ase_file_write_layers(FILE *f, Layer *layer);
static void ase_file_write_cels(FILE *f, Sprite *sprite, Layer *layer, int frpos);

static void ase_file_read_padding(FILE *f, int bytes);
static void ase_file_write_padding(FILE *f, int bytes);
static char *ase_file_read_string(FILE *f);
static void ase_file_write_string(FILE *f, const char *string);

static void ase_file_write_start_chunk(FILE *f, int type);
static void ase_file_write_close_chunk(FILE *f);

static void ase_file_read_color_chunk(FILE *f, RGB *pal);
static void ase_file_write_color_chunk(FILE *f, RGB *pal);
static Layer *ase_file_read_layer_chunk(FILE *f, Sprite *sprite, Layer **previous_layer, int *current_level);
static void ase_file_write_layer_chunk(FILE *f, Layer *layer);
static Cel *ase_file_read_cel_chunk(FILE *f, Sprite *sprite, int frpos, int imgtype);
static void ase_file_write_cel_chunk(FILE *f, Cel *cel, Layer *layer, Sprite *sprite);
static Mask *ase_file_read_mask_chunk(FILE *f);
static void ase_file_write_mask_chunk(FILE *f, Mask *mask);

FileFormat format_ase =
{
  "ase",
  "ase",
  load_ASE,
  save_ASE,
  FILE_SUPPORT_RGB |
  FILE_SUPPORT_RGBA |
  FILE_SUPPORT_GRAY |
  FILE_SUPPORT_GRAYA |
  FILE_SUPPORT_INDEXED |
  FILE_SUPPORT_LAYERS |
  FILE_SUPPORT_FRAMES |
  FILE_SUPPORT_PALETTES |
  FILE_SUPPORT_MASKS_REPOSITORY |
  FILE_SUPPORT_PATHS_REPOSITORY
};

static Sprite *load_ASE(const char *filename)
{
  return ase_file_read(filename);
}

static int save_ASE(Sprite *sprite)
{
  return ase_file_write(sprite);
}

static Sprite *ase_file_read(const char *filename)
{
  Sprite *sprite = NULL;
  FILE *f;

  f = fopen(filename, "rb");
  if (f) {
    sprite = ase_file_read_f(f);
    fclose(f);
  }

  return sprite;
}

static int ase_file_write(Sprite *sprite)
{
  int ret = -1;
  FILE *f;

  f = fopen(sprite->filename, "wb");
  if (f) {
    ret = ase_file_write_f(f, sprite);
    fclose(f);
  }

  return ret;
}

Sprite *ase_file_read_f(FILE *f)
{
  Sprite *sprite = NULL;
  ASE_Header header;
  ASE_FrameHeader frame_header;
  Layer *last_layer;
  int current_level;
  int frame_pos;
  int chunk_pos;
  int chunk_size;
  int chunk_type;
  int c, frpos;

  if (ase_file_read_header(f, &header) != 0) {
    console_printf(_("Error reading header\n"));
    return NULL;
  }

  /* create the new sprite */
  sprite = sprite_new(header.depth == 32 ? IMAGE_RGB:
		      header.depth == 16 ? IMAGE_GRAYSCALE: IMAGE_INDEXED,
		      header.width, header.height);
  if (!sprite) {
    console_printf(_("Error creating sprite with file spec\n"));
    return NULL;
  }

  /* set frames and speed */
  sprite_set_frames(sprite, header.frames);
  sprite_set_speed(sprite, header.speed);

  /* set background color */
  switch (sprite->imgtype) {
    case IMAGE_RGB:
      sprite_set_bgcolor(sprite, _rgba(header.bgcolor[0],
				       header.bgcolor[1],
				       header.bgcolor[2],
				       header.bgcolor[3]));
      break;
    case IMAGE_GRAYSCALE:
      sprite_set_bgcolor(sprite, _graya(header.bgcolor[0],
					header.bgcolor[1]));
      break;
    case IMAGE_INDEXED:
      sprite_set_bgcolor(sprite, header.bgcolor[0]);
      break;
  }

  /* prepare variables for layer chunks */
  last_layer = sprite->set;
  current_level = -1;

  /* progress */
  add_progress(header.size);

  /* read frame by frame to end-of-file */
  for (frpos=0; frpos<sprite->frames; frpos++) {
    /* start frame position */
    frame_pos = ftell(f);
    do_progress(frame_pos);

    /* read frame header */
    ase_file_read_frame_header(f, &frame_header);

    /* correct frame type */
    if (frame_header.magic == ASE_FILE_FRAME_MAGIC) {
      /* use frame-duration field? */
      if (frame_header.duration > 0)
	sprite_set_frlen(sprite, frame_header.duration, frpos);

      /* read chunks */
      for (c=0; c<frame_header.chunks; c++) {
	/* start chunk position */
	chunk_pos = ftell(f);
	do_progress(chunk_pos);

	/* read chunk information */
	chunk_size = fgetl(f);
	chunk_type = fgetw(f);

	switch (chunk_type) {

	  /* only for 8 bpp images */
	  case ASE_FILE_CHUNK_FLI_COLOR:
	    /* console_printf ("Color chunk\n"); */

	    if (sprite->imgtype == IMAGE_INDEXED) {
	      /* TODO fix to read palette-per-frame */
	      PALETTE palette;
	      ase_file_read_color_chunk(f, palette);
	      sprite_set_palette(sprite, palette, 0);
	    }
	    else
	      console_printf(_("Warning: was found a color chunk in non-8bpp file\n"));
	    break;

	  case ASE_FILE_CHUNK_LAYER: {
	    /* console_printf("Layer chunk\n"); */

	    ase_file_read_layer_chunk(f, sprite,
				      &last_layer,
				      &current_level);
	    break;
	  }

	  case ASE_FILE_CHUNK_CEL: {
	    /* console_printf("Cel chunk\n"); */

	    ase_file_read_cel_chunk(f, sprite, frpos, sprite->imgtype);
	    break;
	  }

	  case ASE_FILE_CHUNK_MASK: {
	    Mask *mask;

	    /* console_printf("Mask chunk\n"); */

	    mask = ase_file_read_mask_chunk(f);
	    if (mask)
	      sprite_add_mask(sprite, mask);
	    else
	      console_printf(_("Warning: error loading a mask chunk\n"));

	    break;
	  }

	  case ASE_FILE_CHUNK_PATH:
	    /* console_printf("Path chunk\n"); */
	    break;

	  default:
	    console_printf(_("Warning: Unsupported chunk type %d (skipping)\n"), chunk_type);
	    break;
	}

	/* skip chunk size */
	fseek(f, chunk_pos+chunk_size, SEEK_SET);
      }
    }

    /* skip frame size */
    fseek(f, frame_pos+frame_header.size, SEEK_SET);
  }

  del_progress();

  return sprite;
}

int ase_file_write_f(FILE *f, Sprite *sprite)
{
  ASE_Header header;
  ASE_FrameHeader frame_header;
  JLink link;
  int frpos;

  /* prepare the header */
  ase_file_prepare_header(f, &header, sprite);

  /* add a new progress bar */
  add_progress(sprite->frames);

  /* write frame */
  for (frpos=0; frpos<sprite->frames; frpos++) {
    /* progress */
    do_progress(frpos);

    /* prepare the header */
    ase_file_prepare_frame_header(f, &frame_header);

    /* frame duration */
    frame_header.duration = sprite_get_frlen(sprite, frpos);

    /* write extra chunks in the first frame */
    if (frpos == 0) {
      /* color chunk */
      if (sprite->imgtype == IMAGE_INDEXED)
	/* TODO fix this to write palette per-frame */
	ase_file_write_color_chunk(f, sprite_get_palette(sprite, 0));

      /* write layer chunks */
      JI_LIST_FOR_EACH(sprite->set->layers, link)
	ase_file_write_layers(f, link->data);

      /* write masks */
      JI_LIST_FOR_EACH(sprite->repository.masks, link)
	ase_file_write_mask_chunk(f, link->data);
    }

    /* write cel chunks */
    ase_file_write_cels(f, sprite, sprite->set, frpos);

    /* write the frame header */
    ase_file_write_frame_header(f, &frame_header);
  }

  del_progress();

  /* write the header */
  ase_file_write_header(f, &header);
  return 0;
}

static int ase_file_read_header(FILE *f, ASE_Header *header)
{
  header->pos = ftell(f);

  header->size  = fgetl(f);
  header->magic = fgetw(f);
  if (header->magic != ASE_FILE_MAGIC)
    return -1;

  header->frames     = fgetw(f);
  header->width      = fgetw(f);
  header->height     = fgetw(f);
  header->depth      = fgetw(f);
  header->flags      = fgetl(f);
  header->speed      = fgetw(f);
  header->next       = fgetl(f);
  header->frit       = fgetl(f);
  header->bgcolor[0] = fgetc(f);
  header->bgcolor[1] = fgetc(f);
  header->bgcolor[2] = fgetc(f);
  header->bgcolor[3] = fgetc(f);

  fseek(f, header->pos+128, SEEK_SET);
  return 0;
}

static void ase_file_prepare_header(FILE *f, ASE_Header *header, Sprite *sprite)
{
  header->pos = ftell(f);

  header->size = 0;
  header->magic = ASE_FILE_MAGIC;
  header->frames = sprite->frames;
  header->width = sprite->w;
  header->height = sprite->h;
  header->depth = sprite->imgtype == IMAGE_RGB ? 32:
		  sprite->imgtype == IMAGE_GRAYSCALE ? 16:
                  sprite->imgtype == IMAGE_INDEXED ? 8: 0;
  header->flags = 0;
  header->speed = sprite_get_frlen(sprite, 0);
  header->next = 0;
  header->frit = 0;
  header->bgcolor[0] = 0;
  header->bgcolor[1] = 0;
  header->bgcolor[2] = 0;
  header->bgcolor[3] = 0;

  switch (sprite->imgtype) {
    case IMAGE_RGB:
      header->bgcolor[0] = _rgba_getr(sprite->bgcolor);
      header->bgcolor[1] = _rgba_getg(sprite->bgcolor);
      header->bgcolor[2] = _rgba_getb(sprite->bgcolor);
      header->bgcolor[3] = _rgba_geta(sprite->bgcolor);
      break;
    case IMAGE_GRAYSCALE:
      header->bgcolor[0] = _graya_getk(sprite->bgcolor);
      header->bgcolor[1] = _graya_geta(sprite->bgcolor);
      break;
    case IMAGE_INDEXED:
      header->bgcolor[0] = sprite->bgcolor;
      break;
  }

  fseek(f, header->pos+128, SEEK_SET);
}

static void ase_file_write_header(FILE *f, ASE_Header *header)
{
  header->size = ftell (f)-header->pos;

  fseek(f, header->pos, SEEK_SET);

  fputl(header->size, f);
  fputw(header->magic, f);
  fputw(header->frames, f);
  fputw(header->width, f);
  fputw(header->height, f);
  fputw(header->depth, f);
  fputl(header->flags, f);
  fputw(header->speed, f);
  fputl(header->next, f);
  fputl(header->frit, f);
  fputc(header->bgcolor[0], f);
  fputc(header->bgcolor[1], f);
  fputc(header->bgcolor[2], f);
  fputc(header->bgcolor[3], f);

  ase_file_write_padding(f, 96);

  fseek(f, header->pos+header->size, SEEK_SET);
}

static void ase_file_read_frame_header(FILE *f, ASE_FrameHeader *frame_header)
{
  frame_header->size = fgetl(f);
  frame_header->magic = fgetw(f);
  frame_header->chunks = fgetw(f);
  frame_header->duration = fgetw(f);
  ase_file_read_padding(f, 6);
}

static void ase_file_prepare_frame_header(FILE *f, ASE_FrameHeader *frame_header)
{
  int pos = ftell(f);

  frame_header->size = pos;
  frame_header->magic = ASE_FILE_FRAME_MAGIC;
  frame_header->chunks = 0;
  frame_header->duration = 0;

  current_frame_header = frame_header;

  fseek(f, pos+16, SEEK_SET);
}

static void ase_file_write_frame_header(FILE *f, ASE_FrameHeader *frame_header)
{
  int pos = frame_header->size;
  int end = ftell(f);

  frame_header->size = end-pos;

  fseek(f, pos, SEEK_SET);

  fputl(frame_header->size, f);
  fputw(frame_header->magic, f);
  fputw(frame_header->chunks, f);
  fputw(frame_header->duration, f);
  ase_file_write_padding(f, 6);

  fseek(f, end, SEEK_SET);

  current_frame_header = NULL;
}

static void ase_file_write_layers(FILE *f, Layer *layer)
{
  ase_file_write_layer_chunk(f, layer);

  if (layer_is_set(layer)) {
    JLink link;
    JI_LIST_FOR_EACH(layer->layers, link)
      ase_file_write_layers(f, link->data);
  }
}

static void ase_file_write_cels(FILE *f, Sprite *sprite, Layer *layer, int frpos)
{
  if (layer_is_image(layer)) {
    Cel *cel = layer_get_cel(layer, frpos);

    if (cel) {
/*       console_printf("New cel in frpos %d, in layer %d\n", */
/* 		     frpos, sprite_layer2index(sprite, layer)); */

      ase_file_write_cel_chunk(f, cel, layer, sprite);
    }
  }

  if (layer_is_set(layer)) {
    JLink link;
    JI_LIST_FOR_EACH(layer->layers, link)
      ase_file_write_cels(f, sprite, link->data, frpos);
  }
}

static void ase_file_read_padding(FILE *f, int bytes)
{
  int c;

  for (c=0; c<bytes; c++)
    fgetc(f);
}

static void ase_file_write_padding(FILE *f, int bytes)
{
  int c;

  for (c=0; c<bytes; c++)
    fputc(0, f);
}

static char *ase_file_read_string(FILE *f)
{
  char *string;
  int c, length;

  length = fgetw(f);
  if (length == EOF)
    return NULL;

  string = jmalloc(length+1);
  if (!string)
    return NULL;

  for (c=0; c<length; c++)
    string[c] = fgetc(f);
  string[c] = 0;

  return string;
}

static void ase_file_write_string(FILE *f, const char *string)
{
  const char *p;

  fputw(strlen(string), f);

  for (p=string; *p; p++)
    fputc(*p, f);
}

static void ase_file_write_start_chunk(FILE *f, int type)
{
  current_frame_header->chunks++;

  chunk_type = type;
  chunk_start = ftell(f);

  fseek(f, chunk_start+6, SEEK_SET);
}

static void ase_file_write_close_chunk(FILE *f)
{
  int chunk_end = ftell(f);
  int chunk_size = chunk_end - chunk_start;

  fseek(f, chunk_start, SEEK_SET);
  fputl(chunk_size, f);
  fputw(chunk_type, f);
  fseek(f, chunk_end, SEEK_SET);
}

static void ase_file_read_color_chunk(FILE *f, RGB *pal)
{
  int i, c, packets, skip, size;

  packets = fgetw(f);	/* number of packets */
  skip = 0;

  /* read all packets */
  for (i=0; i<packets; i++) {
    skip += fgetc(f);
    size = fgetc(f);
    if (!size) size = 256;

    for (c=skip; c<skip+size; c++) {
      pal[c].r = fgetc(f);
      pal[c].g = fgetc(f);
      pal[c].b = fgetc(f);
    }
  }
}

/* writes the original color chunk in FLI files for the entire palette "pal" */
static void ase_file_write_color_chunk(FILE *f, RGB *pal)
{
  int c;

  ase_file_write_start_chunk(f, ASE_FILE_CHUNK_FLI_COLOR);

  fputw(1, f);
  fputc(0, f);
  fputc(0, f);
  for (c=0; c<256; c++) {
    fputc(pal[c].r, f);
    fputc(pal[c].g, f);
    fputc(pal[c].b, f);
  }

  ase_file_write_close_chunk(f);
}

static Layer *ase_file_read_layer_chunk(FILE *f, Sprite *sprite, Layer **previous_layer, int *current_level)
{
  char *name;
  Layer *layer = NULL;
  /* read chunk data */
  int flags;
  int layer_type;
  int child_level;
  int blend_mode;

  flags = fgetw(f);
  layer_type = fgetw(f);
  child_level = fgetw(f);
  fgetw(f);			/* default_width */
  fgetw(f);			/* default_height */
  blend_mode = fgetw(f);

  ase_file_read_padding(f, 4);
  name = ase_file_read_string(f);

  /* image layer */
  if (layer_type == 0) {
    layer = layer_new(sprite);
    if (layer)
      layer_set_blend_mode(layer, blend_mode);
  }
  /* layer set */
  else if (layer_type == 1) {
    layer = layer_set_new(sprite);
  }

  if (layer) {
    /* flags */
    layer->readable = (flags & 1) ? TRUE: FALSE;
    layer->writable = (flags & 2) ? TRUE: FALSE;

    /* name */
    if (name)
      layer_set_name(layer, name);

    /* child level... */

    if (child_level == *current_level)
      layer_add_layer((Layer *)(*previous_layer)->parent, layer);
    else if (child_level > *current_level)
      layer_add_layer((*previous_layer), layer);
    else if (child_level < *current_level)
      layer_add_layer((Layer *)((Layer *)(*previous_layer)->parent)->parent, layer);

    *previous_layer = layer;
    *current_level = child_level;
  }

  return layer;
}

static void ase_file_write_layer_chunk(FILE *f, Layer *layer)
{
  GfxObj *parent;
  int child_level;

  ase_file_write_start_chunk(f, ASE_FILE_CHUNK_LAYER);

  /* flags */
  fputw((layer->readable & 1) | ((layer->writable & 1) << 1), f);

  /* layer type */
  fputw(layer_is_image (layer) ? 0:
	layer_is_set (layer) ? 1: -1, f);

  /* layer child level */
  child_level = -1;
  parent = layer->parent;
  while (parent && parent->type != GFXOBJ_SPRITE) {
    child_level++;
    parent = ((Layer *)parent)->parent;
  }
  fputw(child_level, f);

  /* width, height and blend mode */
  fputw(0, f);
  fputw(0, f);
  fputw(layer_is_image (layer) ? layer->blend_mode: 0, f);

  /* padding */
  ase_file_write_padding(f, 4);

  /* layer name */
  ase_file_write_string(f, layer->name);

  ase_file_write_close_chunk(f);

  /* console_printf ("Layer name \"%s\" child level: %d\n", layer->name, child_level); */
}

static Cel *ase_file_read_cel_chunk(FILE *f, Sprite *sprite, int frpos, int imgtype)
{
  Cel *cel;
  /* read chunk data */
  int layer_index = fgetw(f);
  int x = ((short)fgetw(f));
  int y = ((short)fgetw(f));
  int opacity = fgetc(f);
  int cel_type = fgetw(f);
  Layer *layer;

  ase_file_read_padding(f, 7);

  layer = sprite_index2layer(sprite, layer_index);
  if (!layer) {
    console_printf(_("Frame %d didn't found layer with index %d\n"),
		   frpos, layer_index);
    return NULL;
  }
  /* console_printf ("Layer found: %d -> %s\n", layer_index, layer->name); */

  /* create the new frame */
  cel = cel_new(frpos, 0);
  cel_set_position(cel, x, y);
  cel_set_opacity(cel, opacity);

  switch (cel_type) {

    case ASE_FILE_RAW_CEL: {
      int x, y, r, g, b, a, k;
      Image *image;
      /* read width and height */
      int w = fgetw(f);
      int h = fgetw(f);

      if (w > 0 && h > 0) {
	image = image_new(imgtype, w, h);
	if (!image) {
	  cel_free(cel);
	  /* not enough memory for frame's image */
	  return NULL;
	}

	/* read pixel data */
	switch (image->imgtype) {

	  case IMAGE_RGB:
	    for (y=0; y<image->h; y++) {
	      for (x=0; x<image->w; x++) {
		r = fgetc(f);
		g = fgetc(f);
		b = fgetc(f);
		a = fgetc(f);
		image->method->putpixel(image, x, y, _rgba(r, g, b, a));
	      }
	    }
	    do_progress(ftell(f));
	    break;

	  case IMAGE_GRAYSCALE:
	    for (y=0; y<image->h; y++) {
	      for (x=0; x<image->w; x++) {
		k = fgetc(f);
		a = fgetc(f);
		image->method->putpixel(image, x, y, _graya(k, a));
	      }
	    }
	    do_progress(ftell(f));
	    break;

	  case IMAGE_INDEXED:
	    for (y=0; y<image->h; y++) {
	      for (x=0; x<image->w; x++)
		image->method->putpixel(image, x, y, fgetc(f));
	    }
	    do_progress(ftell(f));
	    break;
	}

	cel->image = stock_add_image(sprite->stock, image);
      }
      break;
    }

    case ASE_FILE_LINK_CEL: {
      /* read link position */
      int link_frpos = fgetw(f);
      Cel *link = layer_get_cel(layer, link_frpos);

      if (link)
	cel->image = link->image;
      else {
	cel_free(cel);
	/* linked cel doesn't found */
	return NULL;
      }
      break;
    }

    case ASE_FILE_RLE_COMPRESSED_CEL:
      /* TODO */
      break;
  }

  layer_add_cel(layer, cel);
  return cel;
}

static void ase_file_write_cel_chunk(FILE *f, Cel *cel, Layer *layer, Sprite *sprite)
{
  int layer_index = sprite_layer2index(sprite, layer);
  Cel *link = cel_is_link(cel, layer);
  int cel_type = link ? ASE_FILE_LINK_CEL: ASE_FILE_RAW_CEL;

  ase_file_write_start_chunk(f, ASE_FILE_CHUNK_CEL);

  fputw(layer_index, f);
  fputw(cel->x, f);
  fputw(cel->y, f);
  fputc(cel->opacity, f);
  fputw(cel_type, f);
  ase_file_write_padding(f, 7);

  switch (cel_type) {

    case ASE_FILE_RAW_CEL: {
      Image *image = stock_get_image(sprite->stock, cel->image);
      int x, y, c;

      if (image) {
	/* width and height */
	fputw(image->w, f);
	fputw(image->h, f);

	/* TODO */
/* 	add_progress(image->h); */
 
	/* pixel data */
	switch (image->imgtype) {

	  case IMAGE_RGB:
	    for (y=0; y<image->h; y++) {
	      for (x=0; x<image->w; x++) {
		c = image->method->getpixel(image, x, y);
		fputc(_rgba_getr(c), f);
		fputc(_rgba_getg(c), f);
		fputc(_rgba_getb(c), f);
		fputc(_rgba_geta(c), f);
	      }
/* 	      do_progress(y); */
	    }
	    break;

	  case IMAGE_GRAYSCALE:
	    for (y=0; y<image->h; y++) {
	      for (x=0; x<image->w; x++) {
		c = image->method->getpixel(image, x, y);
		fputc(_graya_getk(c), f);
		fputc(_graya_geta(c), f);
	      }
/* 	      do_progress(y); */
	    }
	    break;

	  case IMAGE_INDEXED:
	    for (y=0; y<image->h; y++) {
	      for (x=0; x<image->w; x++)
		fputc(image->method->getpixel(image, x, y), f);

/* 	      do_progress(y); */
	    }
	    break;
	}

/* 	del_progress(); */
      }
      else {
	/* width and height */
	fputw(0, f);
	fputw(0, f);
      }
      break;
    }

    case ASE_FILE_LINK_CEL:
      /* linked cel to another frame */
      fputw(link->frame, f);
      break;

    case ASE_FILE_RLE_COMPRESSED_CEL:
      /* TODO */
      break;
  }

  ase_file_write_close_chunk(f);
}

static Mask *ase_file_read_mask_chunk(FILE *f)
{
  int c, u, v, byte;
  Mask *mask;
  /* read chunk data */
  int x = fgetw(f);
  int y = fgetw(f);
  int w = fgetw(f);
  int h = fgetw(f);
  char *name;
  ase_file_read_padding(f, 8);
  name = ase_file_read_string(f);

  mask = mask_new();
  if (!mask)
    return NULL;

  if (name)
    mask_set_name(mask, name);

  mask_replace(mask, x, y, w, h);

  /* read image data */
  for (v=0; v<h; v++)
    for (u=0; u<(w+7)/8; u++) {
      byte = fgetc(f);
      for (c=0; c<8; c++)
	image_putpixel(mask->bitmap, u*8+c, v, byte & (1<<(7-c)));
    }

  return mask;
}

static void ase_file_write_mask_chunk(FILE *f, Mask *mask)
{
  int c, u, v, byte;

  ase_file_write_start_chunk(f, ASE_FILE_CHUNK_MASK);

  fputw(mask->x, f);
  fputw(mask->y, f);
  fputw(mask->w, f);
  fputw(mask->h, f);
  ase_file_write_padding(f, 8);

  /* name */
  ase_file_write_string(f, mask->name);

  /* bit map */
  for (v=0; v<mask->h; v++)
    for (u=0; u<(mask->w+7)/8; u++) {
      byte = 0;
      for (c=0; c<8; c++)
	if (image_getpixel(mask->bitmap, u*8+c, v))
	  byte |= (1<<(7-c));
      fputc(byte, f);
    }

  ase_file_write_close_chunk(f);
}

/* returns a word (16 bits) */
int fgetw(FILE *file)
{
  int b1, b2;

  b1 = fgetc(file);
  if (b1 == EOF)
    return EOF;

  b2 = fgetc(file);
  if (b2 == EOF)
    return EOF;

  /* little endian */
  return ((b2 << 8) | b1);
}

/* returns a dword (32 bits) */
long fgetl(FILE *file)
{
  int b1, b2, b3, b4;

  b1 = fgetc(file);
  if (b1 == EOF)
    return EOF;

  b2 = fgetc(file);
  if (b2 == EOF)
    return EOF;

  b3 = fgetc(file);
  if (b3 == EOF)
    return EOF;

  b4 = fgetc(file);
  if (b4 == EOF)
    return EOF;

  /* little endian */
  return ((b4 << 24) | (b3 << 16) | (b2 << 8) | b1);
}

/* returns 0 in success or -1 in error */
int fputw(int w, FILE *file)
{
  int b1, b2;

  /* little endian */
  b2 = (w & 0xFF00) >> 8;
  b1 = w & 0x00FF;

  if (fputc(b1, file) == b1)
    if (fputc(b2, file) == b2)
      return 0;

  return -1;
}

/* returns 0 in success or -1 in error */
int fputl(long l, FILE *file)
{
  int b1, b2, b3, b4;

  /* little endian */
  b4 = (int)((l & 0xFF000000L) >> 24);
  b3 = (int)((l & 0x00FF0000L) >> 16);
  b2 = (int)((l & 0x0000FF00L) >> 8);
  b1 = (int)l & 0x00FF;

  if (fputc(b1, file) == b1)
    if (fputc(b2, file) == b2)
      if (fputc(b3, file) == b3)
	if (fputc(b4, file) == b4)
	  return 0;

  return -1;
}

#if 0
/* returns a floating point (32 bits) */
static float fgetf(FILE *file)
{
  union {
    float f;
    long l;
  } packet;

  packet.l = fgetl(file);

  return packet.f;
}

/* returns a floating point with double precision (64 bits) */
static double fgetd(FILE *file)
{
  union {
    double d;
    long l[2];
  } packet;

  packet.l[0] = fgetl(file);
  packet.l[1] = fgetl(file);

  return packet.d;
}

/* returns 0 in success or -1 in error */
static int fputf(float c, FILE *file)
{
  union {
    float f;
    long l;
  } packet;

  packet.f = c;
  return fputl(packet.l, file);
}

/* returns 0 in success or -1 in error */
static int fputd(double c, FILE *file)
{
  union {
    double d;
    long l[2];
  } packet;

  packet.d = c;

  if (fputl(packet.l[0], file) == 0)
    if (fputl(packet.l[1], file) == 0)
      return 0;

  return -1;
}
#endif
