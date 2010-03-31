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

#include <allegro/color.h>
#include <stdio.h>

#include "file/file.h"
#include "file/fli/fli.h"
#include "modules/palettes.h"
#include "raster/raster.h"

static bool load_FLI(FileOp *fop);
static bool save_FLI(FileOp *fop);

static int get_time_precision(Sprite *sprite);

FileFormat format_fli =
{
  "flc",
  "flc,fli",
  load_FLI,
  save_FLI,
  NULL,
  FILE_SUPPORT_INDEXED |
  FILE_SUPPORT_FRAMES |
  FILE_SUPPORT_PALETTES
};

/* loads a FLI/FLC file */
static bool load_FLI(FileOp *fop)
{
#define SETPAL()						\
  do {								\
      for (c=0; c<256; c++) {					\
	pal->setEntry(c, _rgba(cmap[c*3],			\
			       cmap[c*3+1],			\
			       cmap[c*3+2], 255));		\
      }								\
      pal->setFrame(frpos_out);					\
      sprite->setPalette(pal, true);				\
    } while (0)

  unsigned char cmap[768];
  unsigned char omap[768];
  s_fli_header fli_header;
  Image *bmp, *old, *image;
  Sprite *sprite;
  LayerImage *layer;
  Palette *pal;
  int c, w, h;
  int frpos_in;
  int frpos_out;
  int index = 0;
  Cel *cel;
  FILE *f;

  /* open the file to read in binary mode */
  f = fopen(fop->filename, "rb");
  if (!f)
    return false;

  fli_read_header(f, &fli_header);
  fseek(f, 128, SEEK_SET);

  if (fli_header.magic == NO_HEADER) {
    fop_error(fop, _("The file doesn't have a FLIC header\n"));
    fclose(f);
    return false;
  }

  /* size by frame */
  w = fli_header.width;
  h = fli_header.height;

  /* create the bitmaps */
  bmp = image_new(IMAGE_INDEXED, w, h);
  old = image_new(IMAGE_INDEXED, w, h);
  pal = new Palette(0, 256);
  if (!bmp || !old || !pal) {
    fop_error(fop, _("Not enough memory.\n"));
    if (bmp) image_free(bmp);
    if (old) image_free(old);
    if (pal) delete pal;
    fclose(f);
    return false;
  }

  // Create the image
  sprite = new Sprite(IMAGE_INDEXED, w, h, 256);
  layer = new LayerImage(sprite);
  sprite->getFolder()->add_layer(layer);
  layer->configure_as_background();

  // Set frames and speed
  sprite->setTotalFrames(fli_header.frames);
  sprite->setDurationForAllFrames(fli_header.speed);

  /* write frame by frame */
  for (frpos_in=frpos_out=0;
       frpos_in<sprite->getTotalFrames();
       frpos_in++) {
    /* read the frame */
    fli_read_frame(f, &fli_header,
		   (unsigned char *)old->dat, omap,
		   (unsigned char *)bmp->dat, cmap);

    /* first frame, or the frames changes, or the palette changes */
    if ((frpos_in == 0) ||
	(image_count_diff(old, bmp))
#ifndef USE_LINK /* TODO this should be configurable through a check-box */
	|| (memcmp(omap, cmap, 768) != 0)
#endif
	) {
      /* the image changes? */
      if (frpos_in != 0)
	frpos_out++;

      /* add the new frame */
      image = image_new_copy(bmp);
      if (!image) {
	fop_error(fop, _("Not enough memory\n"));
	break;
      }

      index = stock_add_image(sprite->getStock(), image);
      if (index < 0) {
	image_free(image);
	fop_error(fop, _("Not enough memory\n"));
	break;
      }

      cel = cel_new(frpos_out, index);
      if (!cel) {
	fop_error(fop, _("Not enough memory\n"));
	break;
      }
      layer->add_cel(cel);

      /* first frame or the palette changes */
      if ((frpos_in == 0) || (memcmp(omap, cmap, 768) != 0))
	SETPAL();
    }
#ifdef USE_LINK
    /* the palette changes */
    else if (memcmp(omap, cmap, 768) != 0) {
      frpos_out++;
      SETPAL();

      /* add link */
      cel = cel_new(frpos_out, index);
      if (!cel) {
	fop_error(fop, _("Not enough memory\n"));
	break;
      }

      layer_add_cel(layer, cel);
    }
#endif
    // The palette and the image don't change: add duration to the last added frame
    else {
      sprite->setFrameDuration(frpos_out,
			       sprite->getFrameDuration(frpos_out)+fli_header.speed);
    }

    /* update the old image and color-map to the new ones to compare later */
    image_copy(old, bmp, 0, 0);
    memcpy(omap, cmap, 768);

    /* update progress */
    fop_progress(fop, (float)(frpos_in+1) / (float)(sprite->getTotalFrames()));
    if (fop_is_stop(fop))
      break;

    /* just one frame? */
    if (fop->oneframe)
      break;
  }

  // Update number of frames
  sprite->setTotalFrames(frpos_out+1);

  // Close the file
  fclose(f);

  // Destroy the bitmaps
  image_free(bmp);
  image_free(old);
  delete pal;

  fop->sprite = sprite;
  return true;
}

/* saves a FLC file */
static bool save_FLI(FileOp *fop)
{
  Sprite *sprite = fop->sprite;
  unsigned char cmap[768];
  unsigned char omap[768];
  s_fli_header fli_header;
  int c, frpos, times;
  Image *bmp, *old;
  Palette *pal;
  FILE *f;

  /* prepare fli header */
  fli_header.filesize = 0;
  fli_header.frames = 0;
  fli_header.width = sprite->getWidth();
  fli_header.height = sprite->getHeight();

  if ((fli_header.width == 320) && (fli_header.height == 200))
    fli_header.magic = HEADER_FLI;
  else
    fli_header.magic = HEADER_FLC;

  fli_header.depth = 8;
  fli_header.flags = 3;
  fli_header.speed = get_time_precision(sprite);
  fli_header.created = 0;
  fli_header.updated = 0;
  fli_header.aspect_x = 1;
  fli_header.aspect_y = 1;
  fli_header.oframe1 = fli_header.oframe2 = 0;

  /* open the file to write in binary mode */
  f = fopen(fop->filename, "wb");
  if (!f)
    return false;

  fseek(f, 128, SEEK_SET);

  /* create the bitmaps */
  bmp = image_new(IMAGE_INDEXED, sprite->getWidth(), sprite->getHeight());
  old = image_new(IMAGE_INDEXED, sprite->getWidth(), sprite->getHeight());
  if ((!bmp) || (!old)) {
    fop_error(fop, _("Not enough memory for temporary bitmaps.\n"));
    if (bmp) image_free(bmp);
    if (old) image_free(old);
    fclose(f);
    return false;
  }

  /* write frame by frame */
  for (frpos=0;
       frpos<sprite->getTotalFrames();
       frpos++) {
    /* get color map */
    pal = sprite->getPalette(frpos);
    for (c=0; c<256; c++) {
      cmap[3*c  ] = _rgba_getr(pal->getEntry(c));
      cmap[3*c+1] = _rgba_getg(pal->getEntry(c));
      cmap[3*c+2] = _rgba_getb(pal->getEntry(c));
    }

    /* render the frame in the bitmap */
    image_clear(bmp, 0);
    layer_render(sprite->getFolder(), bmp, 0, 0, frpos);

    /* how many times this frame should be written to get the same
       time that it has in the sprite */
    times = sprite->getFrameDuration(frpos) / fli_header.speed;

    for (c=0; c<times; c++) {
      /* write this frame */
      if (frpos == 0 && c == 0)
	fli_write_frame(f, &fli_header, NULL, NULL,
			(unsigned char *)bmp->dat, cmap, W_ALL);
      else
	fli_write_frame(f, &fli_header,
			(unsigned char *)old->dat, omap,
			(unsigned char *)bmp->dat, cmap, W_ALL);

      /* update the old image and color-map to the new ones to compare later */
      image_copy(old, bmp, 0, 0);
      memcpy(omap, cmap, 768);
    }

    /* update progress */
    fop_progress(fop, (float)(frpos+1) / (float)(sprite->getTotalFrames()));
  }

  /* write the header and close the file */
  fli_write_header(f, &fli_header);
  fclose(f);

  /* destroy the bitmaps */
  image_free(bmp);
  image_free(old);

  return true;
}

static int get_time_precision(Sprite *sprite)
{
  int precision = 1000;
  int c, len;

  for (c = 0; c < sprite->getTotalFrames() && precision > 1; c++) {
    len = sprite->getFrameDuration(c);

    while (len / precision == 0)
      precision /= 10;
  }

  return precision;
}

