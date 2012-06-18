/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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
#include <allegro/draw.h>
#include <allegro/gfx.h>

#include "modules/gfx.h"
#include "raster/blend.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/palette.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "ui/list.h"
#include "util/thmbnail.h"

#define THUMBNAIL_W     32
#define THUMBNAIL_H     32

struct Thumbnail
{
  const Cel *cel;
  BITMAP* bmp;

  Thumbnail(const Cel *cel, BITMAP* bmp)
    : cel(cel), bmp(bmp) {
  }

  ~Thumbnail() {
    destroy_bitmap(bmp);
  }
};

typedef std::vector<Thumbnail*> ThumbnailsList;

static ThumbnailsList* thumbnails = NULL;

static void thumbnail_render(BITMAP* bmp, const Image* image, bool has_alpha, const Palette* palette);

void destroy_thumbnails()
{
  if (thumbnails) {
    for (ThumbnailsList::iterator it = thumbnails->begin(); it != thumbnails->end(); ++it)
      delete *it;

    thumbnails->clear();
    delete thumbnails;
    thumbnails = NULL;
  }
}

BITMAP* generate_thumbnail(const Layer* layer, const Cel* cel, const Sprite *sprite)
{
  Thumbnail* thumbnail;
  BITMAP* bmp;

  if (!thumbnails)
    thumbnails = new ThumbnailsList();

  // Find the thumbnail
  for (ThumbnailsList::iterator it = thumbnails->begin(); it != thumbnails->end(); ++it) {
    thumbnail = *it;
    if (thumbnail->cel == cel)
      return thumbnail->bmp;
  }

  bmp = create_bitmap(THUMBNAIL_W, THUMBNAIL_H);
  if (!bmp)
    return NULL;

  thumbnail_render(bmp,
                   sprite->getStock()->getImage(cel->getImage()),
                   !layer->is_background(),
                   sprite->getPalette(cel->getFrame()));

  thumbnail = new Thumbnail(cel, bmp);
  if (!thumbnail) {
    destroy_bitmap(bmp);
    return NULL;
  }

  thumbnails->push_back(thumbnail);
  return thumbnail->bmp;
}

static void thumbnail_render(BITMAP* bmp, const Image* image, bool has_alpha, const Palette* palette)
{
  register int c, x, y;
  int w, h, x1, y1;
  double sx, sy, scale;

  ASSERT(image != NULL);

  sx = (double)image->w / (double)bmp->w;
  sy = (double)image->h / (double)bmp->h;
  scale = MAX(sx, sy);

  w = image->w / scale;
  h = image->h / scale;
  w = MIN(bmp->w, w);
  h = MIN(bmp->h, h);

  x1 = bmp->w/2 - w/2;
  y1 = bmp->h/2 - h/2;
  x1 = MAX(0, x1);
  y1 = MAX(0, y1);

  /* with alpha blending */
  if (has_alpha) {
    register int c2;

    rectgrid(bmp, 0, 0, bmp->w-1, bmp->h-1,
             bmp->w/4, bmp->h/4);

    switch (image->getPixelFormat()) {
      case IMAGE_RGB:
        for (y=0; y<h; y++)
          for (x=0; x<w; x++) {
            c = image_getpixel(image, x*scale, y*scale);
            c2 = getpixel(bmp, x1+x, y1+y);
            c = _rgba_blend_normal(_rgba(getr(c2), getg(c2), getb(c2), 255), c, 255);

            putpixel(bmp, x1+x, y1+y, makecol(_rgba_getr(c),
                                              _rgba_getg(c),
                                              _rgba_getb(c)));
          }
        break;
      case IMAGE_GRAYSCALE:
        for (y=0; y<h; y++)
          for (x=0; x<w; x++) {
            c = image_getpixel(image, x*scale, y*scale);
            c2 = getpixel(bmp, x1+x, y1+y);
            c = _graya_blend_normal(_graya(getr(c2), 255), c, 255);

            putpixel(bmp, x1+x, y1+y, makecol(_graya_getv(c),
                                              _graya_getv(c),
                                              _graya_getv(c)));
          }
        break;
      case IMAGE_INDEXED: {
        for (y=0; y<h; y++)
          for (x=0; x<w; x++) {
            c = image_getpixel(image, x*scale, y*scale);
            if (c != 0) {
              ASSERT(c >= 0 && c < palette->size());

              c = palette->getEntry(MID(0, c, palette->size()-1));
              putpixel(bmp, x1+x, y1+y, makecol(_rgba_getr(c),
                                                _rgba_getg(c),
                                                _rgba_getb(c)));
            }
          }
        break;
      }
    }
  }
  /* without alpha blending */
  else {
    clear_to_color(bmp, makecol(128, 128, 128));

    switch (image->getPixelFormat()) {
      case IMAGE_RGB:
        for (y=0; y<h; y++)
          for (x=0; x<w; x++) {
            c = image_getpixel(image, x*scale, y*scale);
            putpixel(bmp, x1+x, y1+y, makecol(_rgba_getr(c),
                                              _rgba_getg(c),
                                              _rgba_getb(c)));
          }
        break;
      case IMAGE_GRAYSCALE:
        for (y=0; y<h; y++)
          for (x=0; x<w; x++) {
            c = image_getpixel(image, x*scale, y*scale);
            putpixel(bmp, x1+x, y1+y, makecol(_graya_getv(c),
                                              _graya_getv(c),
                                              _graya_getv(c)));
          }
        break;
      case IMAGE_INDEXED: {
        for (y=0; y<h; y++)
          for (x=0; x<w; x++) {
            c = image_getpixel(image, x*scale, y*scale);

            ASSERT(c >= 0 && c < palette->size());

            c = palette->getEntry(MID(0, c, palette->size()-1));
            putpixel(bmp, x1+x, y1+y, makecol(_rgba_getr(c),
                                              _rgba_getg(c),
                                              _rgba_getb(c)));
          }
        break;
      }
    }
  }
}
