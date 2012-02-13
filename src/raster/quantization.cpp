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

#include <algorithm>
#include <limits>
#include <vector>

#include "gfx/hsv.h"
#include "gfx/rgb.h"
#include "raster/blend.h"
#include "raster/color_histogram.h"
#include "raster/image.h"
#include "raster/images_collector.h"
#include "raster/palette.h"
#include "raster/quantization.h"
#include "raster/rgbmap.h"
#include "raster/sprite.h"

using namespace gfx;

// Converts a RGB image to indexed with ordered dithering method.
static Image* ordered_dithering(const Image* src_image,
                                int offsetx, int offsety,
                                const RgbMap* rgbmap,
                                const Palette* palette);

static void create_palette_from_bitmaps(const std::vector<Image*>& images, Palette* palette, bool has_background_layer);

Palette* quantization::create_palette_from_rgb(const Sprite* sprite)
{
  bool has_background_layer = (sprite->getBackgroundLayer() != NULL);
  Palette* palette = new Palette(0, 256);
  Image* flat_image;

  ImagesCollector images(sprite,
                         true,   // all layers
                         true,   // all frames,
                         false); // forWrite=false, read only

  // Add a flat image with the current sprite's frame rendered
  flat_image = Image::create(sprite->getPixelFormat(), sprite->getWidth(), sprite->getHeight());
  image_clear(flat_image, 0);
  sprite->render(flat_image, 0, 0);

  // Create an array of images
  size_t nimage = images.size() + 1; // +1 for flat_image
  std::vector<Image*> image_array(nimage);

  size_t c = 0;
  for (ImagesCollector::ItemsIterator it=images.begin(); it!=images.end(); ++it)
    image_array[c++] = it->image();
  image_array[c++] = flat_image; // The 'flat_image'

  // Generate an optimized palette for all images
  create_palette_from_bitmaps(image_array, palette, has_background_layer);

  delete flat_image;
  return palette;
}

Image* quantization::convert_pixel_format(const Image* image,
                                          PixelFormat pixelFormat,
                                          DitheringMethod ditheringMethod,
                                          const RgbMap* rgbmap,
                                          const Palette* palette,
                                          bool has_background_layer)
{
  uint32_t* rgb_address;
  uint16_t* gray_address;
  uint8_t* idx_address;
  uint32_t c;
  int i, r, g, b, size;
  Image *new_image;

  // no convertion
  if (image->getPixelFormat() == pixelFormat)
    return NULL;
  // RGB -> Indexed with ordered dithering
  else if (image->getPixelFormat() == IMAGE_RGB &&
           pixelFormat == IMAGE_INDEXED &&
           ditheringMethod == DITHERING_ORDERED) {
    return ordered_dithering(image, 0, 0, rgbmap, palette);
  }

  new_image = Image::create(pixelFormat, image->w, image->h);
  if (!new_image)
    return NULL;

  size = image->w*image->h;

  switch (image->getPixelFormat()) {

    case IMAGE_RGB:
      rgb_address = (uint32_t*)image->dat;

      switch (new_image->getPixelFormat()) {

        // RGB -> Grayscale
        case IMAGE_GRAYSCALE:
          gray_address = (uint16_t*)new_image->dat;
          for (i=0; i<size; i++) {
            c = *rgb_address;

            g = 255 * Hsv(Rgb(_rgba_getr(c),
                              _rgba_getg(c),
                              _rgba_getb(c))).valueInt() / 100;
            *gray_address = _graya(g, _rgba_geta(c));

            rgb_address++;
            gray_address++;
          }
          break;

        // RGB -> Indexed
        case IMAGE_INDEXED:
          idx_address = new_image->dat;
          for (i=0; i<size; i++) {
            c = *rgb_address;
            r = _rgba_getr(c);
            g = _rgba_getg(c);
            b = _rgba_getb(c);
            if (_rgba_geta(c) == 0)
              *idx_address = 0;
            else
              *idx_address = rgbmap->mapColor(r, g, b);
            rgb_address++;
            idx_address++;
          }
          break;
      }
      break;

    case IMAGE_GRAYSCALE:
      gray_address = (uint16_t*)image->dat;

      switch (new_image->getPixelFormat()) {

        // Grayscale -> RGB
        case IMAGE_RGB:
          rgb_address = (uint32_t*)new_image->dat;
          for (i=0; i<size; i++) {
            c = *gray_address;
            g = _graya_getv(c);
            *rgb_address = _rgba(g, g, g, _graya_geta(c));
            gray_address++;
            rgb_address++;
          }
          break;

        // Grayscale -> Indexed
        case IMAGE_INDEXED:
          idx_address = new_image->dat;
          for (i=0; i<size; i++) {
            c = *gray_address;
            if (_graya_geta(c) == 0)
              *idx_address = 0;
            else
              *idx_address = _graya_getv(c);
            gray_address++;
            idx_address++;
          }
          break;
      }
      break;

    case IMAGE_INDEXED:
      idx_address = image->dat;

      switch (new_image->getPixelFormat()) {

        // Indexed -> RGB
        case IMAGE_RGB:
          rgb_address = (uint32_t*)new_image->dat;
          for (i=0; i<size; i++) {
            c = *idx_address;

            if (c == 0 && !has_background_layer)
              *rgb_address = 0;
            else
              *rgb_address = _rgba(_rgba_getr(palette->getEntry(c)),
                                   _rgba_getg(palette->getEntry(c)),
                                   _rgba_getb(palette->getEntry(c)), 255);
            idx_address++;
            rgb_address++;
          }
          break;

        // Indexed -> Grayscale
        case IMAGE_GRAYSCALE:
          gray_address = (uint16_t*)new_image->dat;
          for (i=0; i<size; i++) {
            c = *idx_address;

            if (c == 0 && !has_background_layer)
              *gray_address = 0;
            else {
              r = _rgba_getr(palette->getEntry(c));
              g = _rgba_getg(palette->getEntry(c));
              b = _rgba_getb(palette->getEntry(c));

              g = 255 * Hsv(Rgb(r, g, b)).valueInt() / 100;
              *gray_address = _graya(g, 255);
            }
            idx_address++;
            gray_address++;
          }
          break;

      }
      break;
  }

  return new_image;
}

/* Based on Gary Oberbrunner: */
/*----------------------------------------------------------------------
 * Color image quantizer, from Paul Heckbert's paper in
 * Computer Graphics, vol.16 #3, July 1982 (Siggraph proceedings),
 * pp. 297-304.
 * By Gary Oberbrunner, copyright c. 1988.
 *----------------------------------------------------------------------
 */

/* Bayer-method ordered dither.  The array line[] contains the
 * intensity values for the line being processed.  As you can see, the
 * ordered dither is much simpler than the error dispersion dither.
 * It is also many times faster, but it is not as accurate and
 * produces cross-hatch * patterns on the output.
 */

static int pattern[8][8] = {
  {  0, 32,  8, 40,  2, 34, 10, 42 }, /* 8x8 Bayer ordered dithering  */
  { 48, 16, 56, 24, 50, 18, 58, 26 }, /* pattern.  Each input pixel   */
  { 12, 44,  4, 36, 14, 46,  6, 38 }, /* is scaled to the 0..63 range */
  { 60, 28, 52, 20, 62, 30, 54, 22 }, /* before looking in this table */
  {  3, 35, 11, 43,  1, 33,  9, 41 }, /* to determine the action.     */
  { 51, 19, 59, 27, 49, 17, 57, 25 },
  { 15, 47,  7, 39, 13, 45,  5, 37 },
  { 63, 31, 55, 23, 61, 29, 53, 21 }
};

#define DIST(r1,g1,b1,r2,g2,b2) (3 * ((r1)-(r2)) * ((r1)-(r2)) +        \
                                 4 * ((g1)-(g2)) * ((g1)-(g2)) +        \
                                 2 * ((b1)-(b2)) * ((b1)-(b2)))

static Image* ordered_dithering(const Image* src_image,
                                int offsetx, int offsety,
                                const RgbMap* rgbmap,
                                const Palette* palette)
{
  int oppr, oppg, oppb, oppnrcm;
  Image *dst_image;
  int dither_const;
  int nr, ng, nb;
  int r, g, b, a;
  int nearestcm;
  int c, x, y;

  dst_image = Image::create(IMAGE_INDEXED, src_image->w, src_image->h);
  if (!dst_image)
    return NULL;

  for (y=0; y<src_image->h; y++) {
    for (x=0; x<src_image->w; x++) {
      c = image_getpixel_fast<RgbTraits>(src_image, x, y);

      r = _rgba_getr(c);
      g = _rgba_getg(c);
      b = _rgba_getb(c);
      a = _rgba_geta(c);

      if (a != 0) {
        nearestcm = rgbmap->mapColor(r, g, b);
        /* rgb values for nearest color */
        nr = _rgba_getr(palette->getEntry(nearestcm));
        ng = _rgba_getg(palette->getEntry(nearestcm));
        nb = _rgba_getb(palette->getEntry(nearestcm));
        /* Color as far from rgb as nrngnb but in the other direction */
        oppr = MID(0, 2*r - nr, 255);
        oppg = MID(0, 2*g - ng, 255);
        oppb = MID(0, 2*b - nb, 255);
        /* Nearest match for opposite color: */
        oppnrcm = rgbmap->mapColor(oppr, oppg, oppb);
        /* If they're not the same, dither between them. */
        /* Dither constant is measured by where the true
           color lies between the two nearest approximations.
           Since the most nearly opposite color is not necessarily
           on the line from the nearest through the true color,
           some triangulation error can be introduced.  In the worst
           case the r-nr distance can actually be less than the nr-oppr
           distance. */
        if (oppnrcm != nearestcm) {
          oppr = _rgba_getr(palette->getEntry(oppnrcm));
          oppg = _rgba_getg(palette->getEntry(oppnrcm));
          oppb = _rgba_getb(palette->getEntry(oppnrcm));

          dither_const = DIST(nr, ng, nb, oppr, oppg, oppb);
          if (dither_const != 0) {
            dither_const = 64 * DIST(r, g, b, nr, ng, nb) / dither_const;
            dither_const = MIN(63, dither_const);

            if (pattern[(x+offsetx) & 7][(y+offsety) & 7] < dither_const)
              nearestcm = oppnrcm;
          }
        }
      }
      else
        nearestcm = 0;

      image_putpixel_fast<IndexedTraits>(dst_image, x, y, nearestcm);
    }
  }

  return dst_image;
}

//////////////////////////////////////////////////////////////////////
// Creation of optimized palette for RGB images
// by David Capello

static void create_palette_from_bitmaps(const std::vector<Image*>& images, Palette* palette, bool has_background_layer)
{
  quantization::ColorHistogram<5, 6, 5> histogram;
  uint32_t color;
  RgbTraits::address_t address;

  // If the sprite has a background layer, the first entry can be
  // used, in other case the 0 indexed will be the mask color, so it
  // will not be used later in the color conversion (from RGB to
  // Indexed).
  int first_usable_entry = (has_background_layer ? 0: 1);

  for (int i=0; i<(int)images.size(); ++i) {
    const Image* image = images[i];

    for (int y=0; y<image->h; ++y) {
      address = image_address_fast<RgbTraits>(image, 0, y);

      for (int x=0; x<image->w; ++x) {
        color = *address;

        if (_rgba_geta(color) > 0) {
          color |= _rgba(0, 0, 0, 255);
          histogram.addSamples(color, 1);
        }

        ++address;
      }
    }
  }

  int used_colors = histogram.createOptimizedPalette(palette, first_usable_entry, 255);
  //palette->resize(first_usable_entry+used_colors);   // TODO
}
