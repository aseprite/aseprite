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

#include <stdio.h>

#include "jinete/jlist.h"

/* #include "file/ase_format.h" */
#include "file/file.h"
#include "raster/raster.h"

#define ASE_FILE_MAGIC			0xA5E0
#define ASE_FILE_FRAME_MAGIC		0xF1FA

#define ASE_FILE_CHUNK_FLI_COLOR2	4
#define ASE_FILE_CHUNK_FLI_COLOR	11
#define ASE_FILE_CHUNK_LAYER		0x2004
#define ASE_FILE_CHUNK_CEL		0x2005
#define ASE_FILE_CHUNK_MASK		0x2016
#define ASE_FILE_CHUNK_PATH		0x2017

#define ASE_FILE_RAW_CEL		0
#define ASE_FILE_LINK_CEL		1
#define ASE_FILE_RLE_COMPRESSED_CEL	2 /* TODO change this with zlib */

typedef struct ASE_Header
{
  long pos;

  ase_uint32 size;
  ase_uint16 magic;
  ase_uint16 frames;
  ase_uint16 width;
  ase_uint16 height;
  ase_uint16 depth;
  ase_uint32 flags;
  ase_uint16 speed;	/* deprecated, use "duration" of FrameHeader */
  ase_uint32 next;
  ase_uint32 frit;
} ASE_Header;

typedef struct ASE_FrameHeader
{
  ase_uint32 size;
  ase_uint16 magic;
  ase_uint16 chunks;
  ase_uint16 duration;
} ASE_FrameHeader;

/* TODO warning: the writing routines aren't thread-safe */
static ASE_FrameHeader *current_frame_header = NULL;
static int chunk_type;
static int chunk_start;

static bool load_ASE(FileOp *fop);
static bool save_ASE(FileOp *fop);

static bool ase_file_read_header(FILE *f, ASE_Header *header);
static void ase_file_prepare_header(FILE *f, ASE_Header *header, Sprite *sprite);
static void ase_file_write_header(FILE *f, ASE_Header *header);

static void ase_file_read_frame_header(FILE *f, ASE_FrameHeader *frame_header);
static void ase_file_prepare_frame_header(FILE *f, ASE_FrameHeader *frame_header);
static void ase_file_write_frame_header(FILE *f, ASE_FrameHeader *frame_header);

static void ase_file_write_layers(FILE *f, Layer *layer);
static void ase_file_write_cels(FILE *f, Sprite *sprite, Layer *layer, int frame);

static void ase_file_read_padding(FILE *f, int bytes);
static void ase_file_write_padding(FILE *f, int bytes);
static std::string ase_file_read_string(FILE *f);
static void ase_file_write_string(FILE *f, const std::string& string);

static void ase_file_write_start_chunk(FILE *f, int type);
static void ase_file_write_close_chunk(FILE *f);

static Palette *ase_file_read_color_chunk(FILE *f, Sprite *sprite, int frame);
static Palette *ase_file_read_color2_chunk(FILE *f, Sprite *sprite, int frame);
static void ase_file_write_color2_chunk(FILE *f, Palette *pal);
static Layer *ase_file_read_layer_chunk(FILE *f, Sprite *sprite, Layer **previous_layer, int *current_level);
static void ase_file_write_layer_chunk(FILE *f, Layer *layer);
static Cel *ase_file_read_cel_chunk(FILE *f, Sprite *sprite, int frame, int imgtype, FileOp *fop, ASE_Header *header);
static void ase_file_write_cel_chunk(FILE *f, Cel *cel, LayerImage *layer, Sprite *sprite);
static Mask *ase_file_read_mask_chunk(FILE *f);
static void ase_file_write_mask_chunk(FILE *f, Mask *mask);

FileFormat format_ase =
{
  "ase,aseprite",
  "ase,aseprite",
  load_ASE,
  save_ASE,
  NULL,
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

static bool load_ASE(FileOp *fop)
{
  Sprite *sprite = NULL;
  ASE_Header header;
  ASE_FrameHeader frame_header;
  int current_level;
  int frame_pos;
  int chunk_pos;
  int chunk_size;
  int chunk_type;
  int c, frame;
  FILE *f;

  f = fopen(fop->filename, "rb");
  if (!f)
    return false;

  if (!ase_file_read_header(f, &header)) {
    fop_error(fop, _("Error reading header\n"));
    fclose(f);
    return false;
  }

  /* create the new sprite */
  sprite = sprite_new(header.depth == 32 ? IMAGE_RGB:
		      header.depth == 16 ? IMAGE_GRAYSCALE: IMAGE_INDEXED,
		      header.width, header.height);
  if (!sprite) {
    fop_error(fop, _("Error creating sprite with file spec\n"));
    fclose(f);
    return false;
  }

  /* set frames and speed */
  sprite_set_frames(sprite, header.frames);
  sprite_set_speed(sprite, header.speed);

  /* prepare variables for layer chunks */
  Layer* last_layer = sprite->get_folder();
  current_level = -1;

  /* read frame by frame to end-of-file */
  for (frame=0; frame<sprite->frames; frame++) {
    /* start frame position */
    frame_pos = ftell(f);
    fop_progress(fop, (float)frame_pos / (float)header.size);

    /* read frame header */
    ase_file_read_frame_header(f, &frame_header);

    /* correct frame type */
    if (frame_header.magic == ASE_FILE_FRAME_MAGIC) {
      /* use frame-duration field? */
      if (frame_header.duration > 0)
	sprite_set_frlen(sprite, frame, frame_header.duration);

      /* read chunks */
      for (c=0; c<frame_header.chunks; c++) {
	/* start chunk position */
	chunk_pos = ftell(f);
	fop_progress(fop, (float)chunk_pos / (float)header.size);

	/* read chunk information */
	chunk_size = fgetl(f);
	chunk_type = fgetw(f);

	switch (chunk_type) {

	  /* only for 8 bpp images */
	  case ASE_FILE_CHUNK_FLI_COLOR:
	  case ASE_FILE_CHUNK_FLI_COLOR2:
	    /* fop_error(fop, "Color chunk\n"); */

	    if (sprite->imgtype == IMAGE_INDEXED) {
	      Palette *prev_pal = sprite_get_palette(sprite, frame);
	      Palette *pal =
		chunk_type == ASE_FILE_CHUNK_FLI_COLOR ? 
		ase_file_read_color_chunk(f, sprite, frame):
		ase_file_read_color2_chunk(f, sprite, frame);

	      if (palette_count_diff(prev_pal, pal, NULL, NULL) > 0) {
		sprite_set_palette(sprite, pal, true);
	      }

	      palette_free(pal);
	    }
	    else
	      fop_error(fop, _("Warning: was found a color chunk in non-8bpp file\n"));
	    break;

	  case ASE_FILE_CHUNK_LAYER: {
	    /* fop_error(fop, "Layer chunk\n"); */

	    ase_file_read_layer_chunk(f, sprite,
				      &last_layer,
				      &current_level);
	    break;
	  }

	  case ASE_FILE_CHUNK_CEL: {
	    /* fop_error(fop, "Cel chunk\n"); */

	    ase_file_read_cel_chunk(f, sprite, frame,
				    sprite->imgtype, fop, &header);
	    break;
	  }

	  case ASE_FILE_CHUNK_MASK: {
	    Mask *mask;

	    /* fop_error(fop, "Mask chunk\n"); */

	    mask = ase_file_read_mask_chunk(f);
	    if (mask)
	      sprite_add_mask(sprite, mask);
	    else
	      fop_error(fop, _("Warning: error loading a mask chunk\n"));

	    break;
	  }

	  case ASE_FILE_CHUNK_PATH:
	    /* fop_error(fop, "Path chunk\n"); */
	    break;

	  default:
	    fop_error(fop, _("Warning: Unsupported chunk type %d (skipping)\n"), chunk_type);
	    break;
	}

	/* skip chunk size */
	fseek(f, chunk_pos+chunk_size, SEEK_SET);
      }
    }

    /* skip frame size */
    fseek(f, frame_pos+frame_header.size, SEEK_SET);

    /* just one frame? */
    if (fop->oneframe)
      break;

    if (fop_is_stop(fop))
      break;
  }

  fop->sprite = sprite;

  if (ferror(f)) {
    fop_error(fop, _("Error reading file.\n"));
    fclose(f);
    return false;
  }
  else {
    fclose(f);
    return true;
  }
}

static bool save_ASE(FileOp *fop)
{
  Sprite *sprite = fop->sprite;
  ASE_Header header;
  ASE_FrameHeader frame_header;
  JLink link;
  int frame;
  FILE *f;

  f = fopen(fop->filename, "wb");
  if (!f)
    return false;

  /* prepare the header */
  ase_file_prepare_header(f, &header, sprite);

  /* write frame */
  for (frame=0; frame<sprite->frames; frame++) {
    /* prepare the header */
    ase_file_prepare_frame_header(f, &frame_header);

    /* frame duration */
    frame_header.duration = sprite_get_frlen(sprite, frame);

    /* the sprite is indexed and the palette changes? (or is the first frame) */
    if (sprite->imgtype == IMAGE_INDEXED &&
	(frame == 0 ||
	 palette_count_diff(sprite_get_palette(sprite, frame-1),
			    sprite_get_palette(sprite, frame), NULL, NULL) > 0)) {
      /* write the color chunk */
      ase_file_write_color2_chunk(f, sprite_get_palette(sprite, frame));
    }

    /* write extra chunks in the first frame */
    if (frame == 0) {
      LayerIterator it = sprite->get_folder()->get_layer_begin();
      LayerIterator end = sprite->get_folder()->get_layer_end();

      /* write layer chunks */
      for (; it != end; ++it)
	ase_file_write_layers(f, *it);

      /* write masks */
      JI_LIST_FOR_EACH(sprite->repository.masks, link)
	ase_file_write_mask_chunk(f, reinterpret_cast<Mask*>(link->data));
    }

    /* write cel chunks */
    ase_file_write_cels(f, sprite, sprite->get_folder(), frame);

    /* write the frame header */
    ase_file_write_frame_header(f, &frame_header);

    /* progress */
    if (sprite->frames > 1)
      fop_progress(fop, (float)(frame+1) / (float)(sprite->frames));
  }

  /* write the header */
  ase_file_write_header(f, &header);

  if (ferror(f)) {
    fop_error(fop, _("Error writing file.\n"));
    fclose(f);
    return false;
  }
  else {
    fclose(f);
    return true;
  }
}

static bool ase_file_read_header(FILE *f, ASE_Header *header)
{
  header->pos = ftell(f);

  header->size  = fgetl(f);
  header->magic = fgetw(f);
  if (header->magic != ASE_FILE_MAGIC)
    return false;

  header->frames     = fgetw(f);
  header->width      = fgetw(f);
  header->height     = fgetw(f);
  header->depth      = fgetw(f);
  header->flags      = fgetl(f);
  header->speed      = fgetw(f);
  header->next       = fgetl(f);
  header->frit       = fgetl(f);

  fseek(f, header->pos+128, SEEK_SET);
  return true;
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

  fseek(f, header->pos+128, SEEK_SET);
}

static void ase_file_write_header(FILE *f, ASE_Header *header)
{
  header->size = ftell(f)-header->pos;

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

  if (layer->is_folder()) {
    LayerIterator it = static_cast<LayerFolder*>(layer)->get_layer_begin();
    LayerIterator end = static_cast<LayerFolder*>(layer)->get_layer_end();

    for (; it != end; ++it)
      ase_file_write_layers(f, *it);
  }
}

static void ase_file_write_cels(FILE *f, Sprite *sprite, Layer *layer, int frame)
{
  if (layer->is_image()) {
    Cel* cel = static_cast<LayerImage*>(layer)->get_cel(frame);
    if (cel) {
/*       fop_error(fop, "New cel in frame %d, in layer %d\n", */
/* 		     frame, sprite_layer2index(sprite, layer)); */

      ase_file_write_cel_chunk(f, cel, static_cast<LayerImage*>(layer), sprite);
    }
  }

  if (layer->is_folder()) {
    LayerIterator it = static_cast<LayerFolder*>(layer)->get_layer_begin();
    LayerIterator end = static_cast<LayerFolder*>(layer)->get_layer_end();

    for (; it != end; ++it)
      ase_file_write_cels(f, sprite, *it, frame);
  }
}

static void ase_file_read_padding(FILE *f, int bytes)
{
  for (int c=0; c<bytes; c++)
    fgetc(f);
}

static void ase_file_write_padding(FILE *f, int bytes)
{
  for (int c=0; c<bytes; c++)
    fputc(0, f);
}

static std::string ase_file_read_string(FILE *f)
{
  int length = fgetw(f);
  if (length == EOF)
    return "";

  std::string string;
  string.reserve(length+1);

  for (int c=0; c<length; c++)
    string.push_back(fgetc(f));

  return string;
}

static void ase_file_write_string(FILE *f, const std::string& string)
{
  fputw(string.size(), f);

  for (size_t c=0; c<string.size(); ++c)
    fputc(string[c], f);
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

static Palette *ase_file_read_color_chunk(FILE *f, Sprite *sprite, int frame)
{
  int i, c, r, g, b, packets, skip, size;
  Palette *pal = palette_new(frame, MAX_PALETTE_COLORS);
  palette_copy_colors(pal, sprite_get_palette(sprite, frame));

  packets = fgetw(f);	/* number of packets */
  skip = 0;

  /* read all packets */
  for (i=0; i<packets; i++) {
    skip += fgetc(f);
    size = fgetc(f);
    if (!size) size = 256;

    for (c=skip; c<skip+size; c++) {
      r = fgetc(f);
      g = fgetc(f);
      b = fgetc(f);
      palette_set_entry(pal, c,
			_rgba(_rgb_scale_6[r],
			      _rgb_scale_6[g],
			      _rgb_scale_6[b], 255));
    }
  }

  return pal;
}

static Palette *ase_file_read_color2_chunk(FILE *f, Sprite *sprite, int frame)
{
  int i, c, r, g, b, packets, skip, size;
  Palette *pal = palette_new(frame, MAX_PALETTE_COLORS);
  palette_copy_colors(pal, sprite_get_palette(sprite, frame));

  packets = fgetw(f);	/* number of packets */
  skip = 0;

  /* read all packets */
  for (i=0; i<packets; i++) {
    skip += fgetc(f);
    size = fgetc(f);
    if (!size) size = 256;

    for (c=skip; c<skip+size; c++) {
      r = fgetc(f);
      g = fgetc(f);
      b = fgetc(f);
      palette_set_entry(pal, c, _rgba(r, g, b, 255));
    }
  }

  return pal;
}

/* writes the original color chunk in FLI files for the entire palette "pal" */
static void ase_file_write_color2_chunk(FILE *f, Palette *pal)
{
  int c, color;

  ase_file_write_start_chunk(f, ASE_FILE_CHUNK_FLI_COLOR2);

  fputw(1, f);
  fputc(0, f);
  fputc(0, f);
  for (c=0; c<MAX_PALETTE_COLORS; c++) {
    color = palette_get_entry(pal, c);

    fputc(_rgba_getr(color), f);
    fputc(_rgba_getg(color), f);
    fputc(_rgba_getb(color), f);
  }

  ase_file_write_close_chunk(f);
}

static Layer *ase_file_read_layer_chunk(FILE *f, Sprite *sprite, Layer **previous_layer, int *current_level)
{
  std::string name;
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
    layer = new LayerImage(sprite);
    static_cast<LayerImage*>(layer)->set_blend_mode(blend_mode);
  }
  /* layer set */
  else if (layer_type == 1) {
    layer = new LayerFolder(sprite);
  }

  if (layer) {
    // flags
    *layer->flags_addr() = flags;

    // name
    layer->set_name(name.c_str());

    // child level...
    if (child_level == *current_level)
      (*previous_layer)->get_parent()->add_layer(layer);
    else if (child_level > *current_level)
      static_cast<LayerFolder*>(*previous_layer)->add_layer(layer);
    else if (child_level < *current_level)
      (*previous_layer)->get_parent()->get_parent()->add_layer(layer);

    *previous_layer = layer;
    *current_level = child_level;
  }

  return layer;
}

static void ase_file_write_layer_chunk(FILE *f, Layer *layer)
{
  ase_file_write_start_chunk(f, ASE_FILE_CHUNK_LAYER);

  /* flags */
  fputw(*layer->flags_addr(), f);

  /* layer type */
  fputw(layer->is_image() ? 0: (layer->is_folder() ? 1: -1), f);

  /* layer child level */
  LayerFolder* parent = layer->get_parent();
  int child_level = -1;
  while (parent != NULL) {
    child_level++;
    parent = parent->get_parent();
  }
  fputw(child_level, f);

  /* width, height and blend mode */
  fputw(0, f);
  fputw(0, f);
  fputw(layer->is_image() ? static_cast<LayerImage*>(layer)->get_blend_mode(): 0, f);

  /* padding */
  ase_file_write_padding(f, 4);

  /* layer name */
  ase_file_write_string(f, layer->get_name());

  ase_file_write_close_chunk(f);

  /* fop_error(fop, "Layer name \"%s\" child level: %d\n", layer->name, child_level); */
}

static Cel *ase_file_read_cel_chunk(FILE *f, Sprite *sprite, int frame, int imgtype, FileOp *fop, ASE_Header *header)
{
  Cel *cel;
  /* read chunk data */
  int layer_index = fgetw(f);
  int x = ((short)fgetw(f));
  int y = ((short)fgetw(f));
  int opacity = fgetc(f);
  int cel_type = fgetw(f);
  Layer* layer;

  ase_file_read_padding(f, 7);

  layer = sprite_index2layer(sprite, layer_index);
  if (!layer) {
    fop_error(fop, _("Frame %d didn't found layer with index %d\n"),
	      frame, layer_index);
    return NULL;
  }
  if (!layer->is_image()) {
    fop_error(fop, _("Invalid ASE file (frame %d in layer %d which does not contain images\n"),
	      frame, layer_index);
    return NULL;
  }

  /* create the new frame */
  cel = cel_new(frame, 0);
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
		image_putpixel_fast<RgbTraits>(image, x, y, _rgba(r, g, b, a));
	      }
	      fop_progress(fop, (float)ftell(f) / (float)header->size);
	    }
	    break;

	  case IMAGE_GRAYSCALE:
	    for (y=0; y<image->h; y++) {
	      for (x=0; x<image->w; x++) {
		k = fgetc(f);
		a = fgetc(f);
		image_putpixel_fast<GrayscaleTraits>(image, x, y, _graya(k, a));
	      }
	      fop_progress(fop, (float)ftell(f) / (float)header->size);
	    }
	    break;

	  case IMAGE_INDEXED:
	    for (y=0; y<image->h; y++) {
	      for (x=0; x<image->w; x++)
		image_putpixel_fast<IndexedTraits>(image, x, y, fgetc(f));

	      fop_progress(fop, (float)ftell(f) / (float)header->size);
	    }
	    break;
	}

	cel->image = stock_add_image(sprite->stock, image);
      }
      break;
    }

    case ASE_FILE_LINK_CEL: {
      /* read link position */
      int link_frame = fgetw(f);
      Cel* link = static_cast<LayerImage*>(layer)->get_cel(link_frame);

      if (link) {
	// create a copy of the linked cel (avoid using links cel)
	Image* image = image_new_copy(stock_get_image(sprite->stock, link->image));
	cel->image = stock_add_image(sprite->stock, image);
      }
      else {
	cel_free(cel);
	// linked cel doesn't found
	return NULL;
      }
      break;
    }

    case ASE_FILE_RLE_COMPRESSED_CEL:
      // TODO
      break;
  }

  static_cast<LayerImage*>(layer)->add_cel(cel);
  return cel;
}

static void ase_file_write_cel_chunk(FILE *f, Cel *cel, LayerImage *layer, Sprite *sprite)
{
  int layer_index = sprite_layer2index(sprite, layer);
  int cel_type = ASE_FILE_RAW_CEL;

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

	/* pixel data */
	switch (image->imgtype) {

	  case IMAGE_RGB:
	    for (y=0; y<image->h; y++) {
	      for (x=0; x<image->w; x++) {
		c = image_getpixel_fast<RgbTraits>(image, x, y);
		fputc(_rgba_getr(c), f);
		fputc(_rgba_getg(c), f);
		fputc(_rgba_getb(c), f);
		fputc(_rgba_geta(c), f);
	      }
	    }
	    break;

	  case IMAGE_GRAYSCALE:
	    for (y=0; y<image->h; y++) {
	      for (x=0; x<image->w; x++) {
		c = image_getpixel_fast<GrayscaleTraits>(image, x, y);
		fputc(_graya_getv(c), f);
		fputc(_graya_geta(c), f);
	      }
	    }
	    break;

	  case IMAGE_INDEXED:
	    for (y=0; y<image->h; y++) {
	      for (x=0; x<image->w; x++)
		fputc(image_getpixel_fast<IndexedTraits>(image, x, y), f);
	    }
	    break;
	}
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
      //fputw(link->frame, f);
      fputw(0, f);
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

  ase_file_read_padding(f, 8);
  std::string name = ase_file_read_string(f);

  mask = mask_new();
  if (!mask)
    return NULL;

  mask_set_name(mask, name.c_str());
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
