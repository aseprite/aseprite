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
 *
 * gif.c - written by Elias Pschernig.
 */

/* You can uncomment this if do you want to load the internal GIF
   structure to watch if the low-level writer/reader are working good */
/* #define LOAD_GIF_STRUCTURE */

#include "config.h"

#include <allegro/color.h>
#include <allegro/file.h>

#include "jinete/jbase.h"

#include "file/file.h"
#include "file/gif/format.h"
#include "modules/palettes.h"
#include "raster/raster.h"
#include "util/autocrop.h"

#include <stdio.h>
#include <stdlib.h>

static bool load_GIF(FileOp *fop);
static bool save_GIF(FileOp *fop);

FileFormat format_gif =
{
  "gif",
  "gif",
  load_GIF,
  save_GIF,
  NULL,
  FILE_SUPPORT_INDEXED |
  FILE_SUPPORT_FRAMES |
  FILE_SUPPORT_PALETTES
};

static void render_gif_frame(GIF_FRAME *frame, Image *image,
			     int x, int y, int w, int h)
{
  int u, v, c;

  for (v = 0; v < h; v++) {
    for (u = 0; u < w; u++) {
      c = frame->bitmap_8_bit[v * w + u];
      /* TODO fix this!!! */
      /* if (c == frame->transparent_index) */
      /*   c = 0; */
      if (c != frame->transparent_index)
	image_putpixel(image, x+u, y+v, c);
    }
  }
}

/* load_GIF:
 * Loads a GIF into a sprite.
 */
static bool load_GIF(FileOp *fop)
{
  GIF_ANIMATION *gif = NULL;
  Sprite *sprite = NULL;
  LayerImage *layer = NULL;
  Cel *cel = NULL;
  Image *image = NULL;
  Image *current_image_old = NULL;
  Image *current_image = NULL;
  Palette *opal = NULL;
  Palette *npal = NULL;
  bool ret = false;
  int i, c;

  gif = gif_load_animation(fop->filename,
			   reinterpret_cast<void(*)(void*,float)>(fop_progress), fop);
  if (!gif) {
    fop_error(fop, _("Error loading GIF file.\n"));
    goto error;
  }

  current_image = image_new(IMAGE_INDEXED, gif->width, gif->height);
  current_image_old = image_new(IMAGE_INDEXED, gif->width, gif->height);
  opal = new Palette(0, 256);
  npal = new Palette(0, 256);
  if (!current_image || !current_image_old || !opal || !npal) {
    fop_error(fop, _("Error creating temporary image.\n"));
    goto error;
  }

  sprite = new Sprite(IMAGE_INDEXED, gif->width, gif->height, 256);
  if (!sprite) {
    fop_error(fop, _("Error creating sprite.\n"));
    goto error;
  }

  sprite->setTotalFrames(gif->frames_count);

  layer = new LayerImage(sprite);
  if (!layer) {
    fop_error(fop, _("Error creating main layer.\n"));
    goto error;
  }

  sprite->getFolder()->add_layer(layer);
  layer->configure_as_background();

  image_clear(current_image, gif->background_index);
  image_clear(current_image_old, gif->background_index);

  for (i = 0; i < gif->frames_count; i++) {
    GIF_PALETTE *pal = &gif->frames[i].palette;

    /* Use global palette if no local. */
    if (pal->colors_count == 0)
      pal = &gif->palette;

    /* 1/100th seconds to milliseconds */
    sprite->setFrameDuration(i, gif->frames[i].duration*10);

    /* make the palette */
    for (c=0; c<pal->colors_count; c++) {
      npal->setEntry(c, _rgba(pal->colors[c].r,
			      pal->colors[c].g,
			      pal->colors[c].b, 255));
    }
    if (i == 0)
      for (; c<256; c++)
	npal->setEntry(c, _rgba(0, 0, 0, 255));
    else
      for (; c<256; c++) {
	npal->setEntry(c, opal->getEntry(c));
      }

    /* first frame or palette changes */
    if (i == 0 || opal->countDiff(npal, NULL, NULL)) {
      npal->setFrame(i);
      sprite->setPalette(npal, true);
    }

    /* copy new palette to old palette */
    npal->copyColorsTo(opal);

    cel = cel_new(i, 0);
    image = image_new(IMAGE_INDEXED,
#ifdef LOAD_GIF_STRUCTURE
		      gif->frames[i].w,
		      gif->frames[i].h
#else
		      sprite->getWidth(), sprite->getHeight()
#endif
		      );
    if (!cel || !image) {
      if (cel) cel_free(cel);
      if (image) image_free(image);
      fop_error(fop, _("Error creating cel %d.\n"), i);
      break;
    }

    cel_set_position(cel,
#ifdef LOAD_GIF_STRUCTURE
		     gif->frames[i].xoff,
		     gif->frames[i].yoff
#else
		     0, 0
#endif
		     );

    image_copy(current_image_old, current_image, 0, 0);
    render_gif_frame(gif->frames+i, current_image,
		     gif->frames[i].xoff,
		     gif->frames[i].yoff,
		     gif->frames[i].w,
		     gif->frames[i].h);

    image_copy(image, current_image,
#ifdef LOAD_GIF_STRUCTURE
	       -gif->frames[i].xoff, -gif->frames[i].yoff
#else
	       0, 0
#endif
	       );
    cel->image = stock_add_image(sprite->getStock(), image);
    layer->add_cel(cel);

#ifdef LOAD_GIF_STRUCTURE
    /* when load the GIF structure, the disposal method is ever
       clear the old image */
    image_rectfill(current_image,
		   gif->frames[i].xoff,
		   gif->frames[i].yoff,
		   gif->frames[i].xoff+gif->frames[i].w-1,
		   gif->frames[i].yoff+gif->frames[i].h-1,
		   gif->background_index);
#else
    /* disposal method */
    switch (gif->frames[i].disposal_method) {

      case 0:			/* no disposal specified */
      case 1:			/* do not dispose */
	/* do nothing */
	break;

      case 2:			/* restore to background color */
      case 3:			/* restore to previous */
	image_rectfill(current_image,
		       gif->frames[i].xoff,
		       gif->frames[i].yoff,
		       gif->frames[i].xoff+gif->frames[i].w-1,
		       gif->frames[i].yoff+gif->frames[i].h-1,
		       gif->background_index);

	/* draw old frame */
	if (gif->frames[i].disposal_method == 3 && i > 0) {
	  Image *tmp = image_crop(current_image_old,
				  gif->frames[i].xoff,
				  gif->frames[i].yoff,
				  gif->frames[i].w,
				  gif->frames[i].h, 0);
	  if (tmp) {
	    image_copy(current_image, tmp,
		       gif->frames[i].xoff,
		       gif->frames[i].yoff);
	    image_free(tmp);
	  }
	}
	break;
    }
#endif
  }

  fop->sprite = sprite;
  sprite = NULL;
  ret = true;

error:;
  if (gif) gif_destroy_animation(gif);
  if (current_image) image_free(current_image);
  if (current_image_old) image_free(current_image_old);
  if (npal) delete npal;
  if (opal) delete opal;
  delete sprite;
  return ret;
}

/* TODO: find the colors that are used and resort the palette */
static int max_used_index(Image *image)
{
  unsigned char *address = image->dat;
  int size = image->w*image->h;
  register int i, c, max = 0;
  for (i=0; i<size; ++i) {
    c = *(address++);
    if (max < c)
      max = c;
  }
  return max;
}

/* save_gif:
 *  Writes a sprite into a GIF file.
 *
 *  TODO: transparent index is not stored. And reserve a single color
 *  as transparent.
 */
static bool save_GIF(FileOp *fop)
{
  Sprite *sprite = fop->sprite;
  GIF_ANIMATION *gif;
  int x1, y1, x2, y2;
  int u1, v1, u2, v2;
  int i1, j1, i2, j2;
  Image *bmp, *old;
  Palette *opal, *npal;
  int c, i, x, y;
  int w, h;
  int ret;

  bmp = image_new(IMAGE_INDEXED, sprite->getWidth(), sprite->getHeight());
  old = image_new(IMAGE_INDEXED, sprite->getWidth(), sprite->getHeight());
  if (!bmp || !old) {
    if (bmp) image_free(bmp);
    if (old) image_free(old);
    fop_error(fop, _("Not enough memory for temporary bitmaps.\n"));
    return false;
  }

  gif = gif_create_animation(sprite->getTotalFrames());
  if (!gif) {
    image_free(bmp);
    image_free(old);
    fop_error(fop, _("Error creating GIF structure.\n"));
    return false;
  }

  gif->width = sprite->getWidth();
  gif->height = sprite->getHeight();
  gif->background_index = 0;

  /* defaults:
     - disposal method: restore to background
     - transparent pixel: 0 */
  for (i = 0; i < gif->frames_count; i++) {
    gif->frames[i].disposal_method = 2;
    gif->frames[i].transparent_index = 0;
  }

  /* avoid compilation warnings */
  x1 = y1 = x2 = y2 = 0;

  opal = NULL;
  for (i=0; i<sprite->getTotalFrames(); ++i) {
    /* frame palette */
    npal = sprite->getPalette(i);

    /* render the frame in the bitmap */
    image_clear(bmp, 0);
    layer_render(sprite->getFolder(), bmp, 0, 0, i);

    /* first frame */
    if (i == 0) {
      /* TODO: don't use 256 colors, but only as much as needed. */
      gif->palette.colors_count = max_used_index(bmp)+1;
      for (c=0; c<gif->palette.colors_count; ++c) {
	gif->palette.colors[c].r = _rgba_getr(npal->getEntry(c));
	gif->palette.colors[c].g = _rgba_getg(npal->getEntry(c));
	gif->palette.colors[c].b = _rgba_getb(npal->getEntry(c));
      }

      /* render all */
      x1 = y1 = 0;
      x2 = bmp->w-1;
      y2 = bmp->h-1;
    }
    /* other frames */
    else {
#if 0 /* TODO remove this (doesn't work right for sprites with trans-color */
      /* get the rectangle where start differences with the previous frame */
      if (!get_shrink_rect2(&u1, &v1, &u2, &v2, bmp, old)) {
	/* there aren't differences with the previous image? */
	/* TODO: in this case, we should remove this frame, and
	   expand the previous frame to the time that this frame is using */
	x1 = gif->frames[i-1].xoff;
	y1 = gif->frames[i-1].yoff;
	x2 = gif->frames[i-1].xoff+gif->frames[i-1].w-1;
	y2 = gif->frames[i-1].yoff+gif->frames[i-1].h-1;
	/* change previous frame disposal method: left in place */
	gif->frames[i-1].disposal_method = 1;
      }
      /* check the minimal area with the background color */
      else if (get_shrink_rect(&x1, &y1, &x2, &y2, bmp,
			       gif->background_index)) {
	int a1 = (x2-x1+1) * (y2-y1+1);
	int a2 = (u2-u1+1) * (v2-v1+1);
	/* differences with previous is better that differences with
	   background color? */
	if (a2 < a1) {
	  x1 = u1;
	  y1 = v1;
	  x2 = u2;
	  y2 = v2;
	  /* change previous frame disposal method: left in place */
	  gif->frames[i-1].disposal_method = 1;
	  /* change this frame to non-transparent */
	  gif->frames[i].transparent_index = -1;
	}
      }
      /* the new image is different of the old one, but is all cleared
	 with the background color */
      else {
	x1 = u1;
	y1 = v1;
	x2 = u2;
	y2 = v2;
	/* change this frame to non-transparent */
	gif->frames[i].transparent_index = -1;
      }
#else
      /* get the rectangle where start differences with the previous frame */
      if (get_shrink_rect2(&u1, &v1, &u2, &v2, bmp, old)) {
	/* check the minimal area with the background color */
	if (get_shrink_rect(&i1, &j1, &i2, &j2, bmp, gif->background_index)) {
	  x1 = MIN(u1, i1);
	  y1 = MIN(v1, j1);
	  x2 = MAX(u2, i2);
	  y2 = MAX(v2, j2);
	}
      }
#endif

      /* palette changes */
      if (opal != npal) {
	gif->frames[i].palette.colors_count = max_used_index(bmp)+1;
	for (c=0; c<gif->frames[i].palette.colors_count; ++c) {
	  gif->frames[i].palette.colors[c].r = _rgba_getr(npal->getEntry(c));
	  gif->frames[i].palette.colors[c].g = _rgba_getg(npal->getEntry(c));
	  gif->frames[i].palette.colors[c].b = _rgba_getb(npal->getEntry(c));
	}
      }
    }

    w = x2-x1+1;
    h = y2-y1+1;

    gif->frames[i].w = w;
    gif->frames[i].h = h;
    gif->frames[i].bitmap_8_bit = (unsigned char*)calloc(w, h);
    gif->frames[i].xoff = x1;
    gif->frames[i].yoff = y1;

    /* milliseconds to 1/100th seconds */
    gif->frames[i].duration = sprite->getFrameDuration(i)/10;

    /* image data */
    for (y = 0; y < h; y++) {
      for (x = 0; x < w; x++) {
	gif->frames[i].bitmap_8_bit[x + y * w] =
	  image_getpixel(bmp, x1+x, y1+y);
      }
    }

    /* update the old image and color-map to the new ones to compare later */
    image_copy(old, bmp, 0, 0);
    opal = npal;
  }

  ret = gif_save_animation(fop->filename, gif,
			   reinterpret_cast<void(*)(void*,float)>(fop_progress), fop);
  gif_destroy_animation(gif);
  return ret == 0 ? true: false;
}
