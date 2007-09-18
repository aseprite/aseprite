/* ase -- allegro-sprite-editor: the ultimate sprites factory
 * Copyright (C) 2001-2005  David A. Capello
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

#include <allegro/color.h>
#include <allegro/draw.h>
#include <allegro/gfx.h>

#include "jinete/list.h"

#include "modules/palette.h"
#include "raster/frame.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/stock.h"
#include "util/thmbnail.h"

#endif

static JList thumbnails = NULL;

static Thumbnail *thumbnail_new(Frame *frame, BITMAP *bmp);
static void thumbnail_free(Thumbnail *thumbnail);
static void thumbnail_create_bitmap(BITMAP *bmp, Image *image);

void destroy_thumbnails(void)
{
  if (thumbnails) {
    JLink link;

    JI_LIST_FOR_EACH(thumbnails, link)
      thumbnail_free(link->data);

    jlist_free(thumbnails);
    thumbnails = NULL;
  }
}

BITMAP *generate_thumbnail(Frame *frame, Layer *layer)
{
  Thumbnail *thumbnail;
  BITMAP *bmp;
  JLink link;

  if (!thumbnails)
    thumbnails = jlist_new();

#if 0				/* this must be done by the caller */
  {
    Frame *link_frame;

    /* search original frame */
    link_frame = frame_is_link(frame, layer);
    if (link_frame)
      frame = link_frame;
  }
#endif

  /* find the thumbnail */
  JI_LIST_FOR_EACH(thumbnails, link) {
    thumbnail = link->data;
    if (thumbnail->frame == frame)
      return thumbnail->bmp;
  }

  bmp = create_bitmap(THUMBNAIL_W, THUMBNAIL_H);
  if (!bmp)
    return NULL;

  thumbnail_create_bitmap(bmp, stock_get_image(layer->stock, frame->image));

  thumbnail = thumbnail_new(frame, bmp);
  if (!thumbnail) {
    destroy_bitmap(bmp);
    return NULL;
  }

  jlist_append(thumbnails, thumbnail);
  return thumbnail->bmp;
}

static Thumbnail *thumbnail_new(Frame *frame, BITMAP *bmp)
{
  Thumbnail *thumbnail;

  thumbnail = jnew (Thumbnail, 1);
  if (!thumbnail)
    return NULL;

  thumbnail->frame = frame;
  thumbnail->bmp = bmp;

  return thumbnail;
}

static void thumbnail_free(Thumbnail *thumbnail)
{
  destroy_bitmap (thumbnail->bmp);
  jfree (thumbnail);
}

static void thumbnail_create_bitmap(BITMAP *bmp, Image *image)
{
  if (!image) {
    clear_to_color (bmp, makecol (128, 128, 128));
    line (bmp, 0, 0, bmp->w-1, bmp->h-1, makecol (0, 0, 0));
    line (bmp, 0, bmp->h-1, bmp->w-1, 0, makecol (0, 0, 0));
  }
  else {
    int c, x, y, w, h, x1, y1;
    double sx, sy, scale;

    sx = (double)image->w / (double)bmp->w;
    sy = (double)image->h / (double)bmp->h;
    scale = MAX (sx, sy);

    w = image->w / scale;
    h = image->h / scale;
    w = MIN (bmp->w, w);
    h = MIN (bmp->h, h);

    x1 = bmp->w/2 - w/2;
    y1 = bmp->h/2 - h/2;
    x1 = MAX (0, x1);
    y1 = MAX (0, y1);

    clear_to_color (bmp, makecol (128, 128, 128));

    switch (image->imgtype) {
      case IMAGE_RGB:
	for (y=0; y<h; y++)
	  for (x=0; x<w; x++) {
	    c = image_getpixel (image, x*scale, y*scale);
	    putpixel (bmp, x1+x, y1+y, makecol (_rgba_getr (c),
						_rgba_getg (c),
						_rgba_getb (c)));
	  }
	break;
      case IMAGE_GRAYSCALE:
	for (y=0; y<h; y++)
	  for (x=0; x<w; x++) {
	    c = image_getpixel (image, x*scale, y*scale);
	    putpixel (bmp, x1+x, y1+y, makecol (_graya_getk (c),
						_graya_getk (c),
						_graya_getk (c)));
	  }
	break;
      case IMAGE_INDEXED: {
	for (y=0; y<h; y++)
	  for (x=0; x<w; x++) {
	    c = image_getpixel (image, x*scale, y*scale);
	    putpixel (bmp, x1+x, y1+y,
		      makecol (_rgb_scale_6[current_palette[c].r],
			       _rgb_scale_6[current_palette[c].g],
			       _rgb_scale_6[current_palette[c].b]));
	  }
	break;
      }
    }
  }
}
