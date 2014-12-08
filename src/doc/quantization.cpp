// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/quantization.h"

#include "gfx/hsv.h"
#include "gfx/rgb.h"
#include "doc/blend.h"
#include "doc/image.h"
#include "doc/image_bits.h"
#include "doc/images_collector.h"
#include "doc/layer.h"
#include "doc/palette.h"
#include "doc/primitives.h"
#include "doc/rgbmap.h"
#include "doc/sprite.h"

#include <algorithm>
#include <limits>
#include <vector>

namespace doc {
namespace quantization {

using namespace gfx;

// Converts a RGB image to indexed with ordered dithering method.
static Image* ordered_dithering(
  const Image* src_image,
  Image* dst_image,
  int offsetx, int offsety,
  const RgbMap* rgbmap,
  const Palette* palette);

Palette* create_palette_from_rgb(
  const Sprite* sprite,
  FrameNumber frameNumber,
  Palette* palette)
{
  if (!palette)
    palette = new Palette(FrameNumber(0), 256);

  bool has_background_layer = (sprite->backgroundLayer() != NULL);
  Image* flat_image;

  ImagesCollector images(sprite->folder(), // All layers
                         frameNumber,         // Ignored, we'll use all frames
                         true,                // All frames,
                         false); // forWrite=false, read only

  // Add a flat image with the current sprite's frame rendered
  flat_image = Image::create(sprite->pixelFormat(), sprite->width(), sprite->height());
  clear_image(flat_image, 0);
  sprite->render(flat_image, 0, 0, frameNumber);

  // Create an array of images
  size_t nimage = images.size() + 1; // +1 for flat_image
  std::vector<Image*> image_array(nimage);

  size_t c = 0;
  for (ImagesCollector::ItemsIterator it=images.begin(); it!=images.end(); ++it)
    image_array[c++] = it->image();
  image_array[c++] = flat_image; // The 'flat_image'

  // Generate an optimized palette for all images
  create_palette_from_images(image_array, palette, has_background_layer);

  delete flat_image;
  return palette;
}

Image* convert_pixel_format(
  const Image* image,
  Image* new_image,
  PixelFormat pixelFormat,
  DitheringMethod ditheringMethod,
  const RgbMap* rgbmap,
  const Palette* palette,
  bool is_background)
{
  if (!new_image)
    new_image = Image::create(pixelFormat, image->width(), image->height());

  // RGB -> Indexed with ordered dithering
  if (image->pixelFormat() == IMAGE_RGB &&
      pixelFormat == IMAGE_INDEXED &&
      ditheringMethod == DITHERING_ORDERED) {
    return ordered_dithering(image, new_image, 0, 0, rgbmap, palette);
  }

  color_t c;
  int r, g, b;

  switch (image->pixelFormat()) {

    case IMAGE_RGB: {
      const LockImageBits<RgbTraits> srcBits(image);
      LockImageBits<RgbTraits>::const_iterator src_it = srcBits.begin(), src_end = srcBits.end();

      switch (new_image->pixelFormat()) {

        // RGB -> RGB
        case IMAGE_RGB:
          new_image->copy(image, 0, 0, 0, 0, image->width(), image->height());
          break;

        // RGB -> Grayscale
        case IMAGE_GRAYSCALE: {
          LockImageBits<GrayscaleTraits> dstBits(new_image, Image::WriteLock);
          LockImageBits<GrayscaleTraits>::iterator dst_it = dstBits.begin();
#ifdef _DEBUG
          LockImageBits<GrayscaleTraits>::iterator dst_end = dstBits.end();
#endif

          for (; src_it != src_end; ++src_it, ++dst_it) {
            ASSERT(dst_it != dst_end);
            c = *src_it;

            g = 255 * Hsv(Rgb(rgba_getr(c),
                              rgba_getg(c),
                              rgba_getb(c))).valueInt() / 100;

            *dst_it = graya(g, rgba_geta(c));
          }
          ASSERT(dst_it == dst_end);
          break;
        }

        // RGB -> Indexed
        case IMAGE_INDEXED: {
          LockImageBits<IndexedTraits> dstBits(new_image, Image::WriteLock);
          LockImageBits<IndexedTraits>::iterator dst_it = dstBits.begin();
#ifdef _DEBUG
          LockImageBits<IndexedTraits>::iterator dst_end = dstBits.end();
#endif

          for (; src_it != src_end; ++src_it, ++dst_it) {
            ASSERT(dst_it != dst_end);
            c = *src_it;

            r = rgba_getr(c);
            g = rgba_getg(c);
            b = rgba_getb(c);

            if (rgba_geta(c) == 0)
              *dst_it = 0;
            else
              *dst_it = rgbmap->mapColor(r, g, b);
          }
          ASSERT(dst_it == dst_end);
          break;
        }
      }
      break;
    }

    case IMAGE_GRAYSCALE: {
      const LockImageBits<GrayscaleTraits> srcBits(image);
      LockImageBits<GrayscaleTraits>::const_iterator src_it = srcBits.begin(), src_end = srcBits.end();

      switch (new_image->pixelFormat()) {

        // Grayscale -> RGB
        case IMAGE_RGB: {
          LockImageBits<RgbTraits> dstBits(new_image, Image::WriteLock);
          LockImageBits<RgbTraits>::iterator dst_it = dstBits.begin();
#ifdef _DEBUG
          LockImageBits<RgbTraits>::iterator dst_end = dstBits.end();
#endif

          for (; src_it != src_end; ++src_it, ++dst_it) {
            ASSERT(dst_it != dst_end);
            c = *src_it;

            g = graya_getv(c);

            *dst_it = rgba(g, g, g, graya_geta(c));
          }
          ASSERT(dst_it == dst_end);
          break;
        }

        // Grayscale -> Grayscale
        case IMAGE_GRAYSCALE:
          new_image->copy(image, 0, 0, 0, 0, image->width(), image->height());
          break;

        // Grayscale -> Indexed
        case IMAGE_INDEXED: {
          LockImageBits<IndexedTraits> dstBits(new_image, Image::WriteLock);
          LockImageBits<IndexedTraits>::iterator dst_it = dstBits.begin();
#ifdef _DEBUG
          LockImageBits<IndexedTraits>::iterator dst_end = dstBits.end();
#endif

          for (; src_it != src_end; ++src_it, ++dst_it) {
            ASSERT(dst_it != dst_end);
            c = *src_it;

            if (graya_geta(c) == 0)
              *dst_it = 0;
            else
              *dst_it = graya_getv(c);
          }
          ASSERT(dst_it == dst_end);
          break;
        }
      }
      break;
    }

    case IMAGE_INDEXED: {
      const LockImageBits<IndexedTraits> srcBits(image);
      LockImageBits<IndexedTraits>::const_iterator src_it = srcBits.begin(), src_end = srcBits.end();

      switch (new_image->pixelFormat()) {

        // Indexed -> RGB
        case IMAGE_RGB: {
          LockImageBits<RgbTraits> dstBits(new_image, Image::WriteLock);
          LockImageBits<RgbTraits>::iterator dst_it = dstBits.begin();
#ifdef _DEBUG
          LockImageBits<RgbTraits>::iterator dst_end = dstBits.end();
#endif

          for (; src_it != src_end; ++src_it, ++dst_it) {
            ASSERT(dst_it != dst_end);
            c = *src_it;

            if (!is_background && c == image->maskColor())
              *dst_it = 0;
            else
              *dst_it = rgba(rgba_getr(palette->getEntry(c)),
                             rgba_getg(palette->getEntry(c)),
                             rgba_getb(palette->getEntry(c)), 255);
          }
          ASSERT(dst_it == dst_end);
          break;
        }

        // Indexed -> Grayscale
        case IMAGE_GRAYSCALE: {
          LockImageBits<GrayscaleTraits> dstBits(new_image, Image::WriteLock);
          LockImageBits<GrayscaleTraits>::iterator dst_it = dstBits.begin();
#ifdef _DEBUG
          LockImageBits<GrayscaleTraits>::iterator dst_end = dstBits.end();
#endif

          for (; src_it != src_end; ++src_it, ++dst_it) {
            ASSERT(dst_it != dst_end);
            c = *src_it;

            if (!is_background && c == image->maskColor())
              *dst_it = 0;
            else {
              r = rgba_getr(palette->getEntry(c));
              g = rgba_getg(palette->getEntry(c));
              b = rgba_getb(palette->getEntry(c));

              g = 255 * Hsv(Rgb(r, g, b)).valueInt() / 100;
              *dst_it = graya(g, 255);
            }
          }
          ASSERT(dst_it == dst_end);
          break;
        }

        // Indexed -> Indexed
        case IMAGE_INDEXED: {
          LockImageBits<IndexedTraits> dstBits(new_image, Image::WriteLock);
          LockImageBits<IndexedTraits>::iterator dst_it = dstBits.begin();
#ifdef _DEBUG
          LockImageBits<IndexedTraits>::iterator dst_end = dstBits.end();
#endif
          color_t dstMaskColor = new_image->maskColor();

          for (; src_it != src_end; ++src_it, ++dst_it) {
            ASSERT(dst_it != dst_end);
            c = *src_it;

            if (!is_background && c == image->maskColor())
              *dst_it = dstMaskColor;
            else {
              r = rgba_getr(palette->getEntry(c));
              g = rgba_getg(palette->getEntry(c));
              b = rgba_getb(palette->getEntry(c));

              *dst_it = rgbmap->mapColor(r, g, b);
            }
          }
          ASSERT(dst_it == dst_end);
          break;
        }

      }
      break;
    }
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

static Image* ordered_dithering(
  const Image* src_image,
  Image* dst_image,
  int offsetx, int offsety,
  const RgbMap* rgbmap,
  const Palette* palette)
{
  int oppr, oppg, oppb, oppnrcm;
  int dither_const;
  int nr, ng, nb;
  int r, g, b, a;
  int nearestcm;
  int x, y;
  color_t c;

  const LockImageBits<RgbTraits> src_bits(src_image);
  LockImageBits<IndexedTraits> dst_bits(dst_image);
  LockImageBits<RgbTraits>::const_iterator src_it = src_bits.begin();
  LockImageBits<IndexedTraits>::iterator dst_it = dst_bits.begin();

  for (y=0; y<src_image->height(); ++y) {
    for (x=0; x<src_image->width(); ++x, ++src_it, ++dst_it) {
      ASSERT(src_it != src_bits.end());
      ASSERT(dst_it != dst_bits.end());

      c = *src_it;

      r = rgba_getr(c);
      g = rgba_getg(c);
      b = rgba_getb(c);
      a = rgba_geta(c);

      if (a != 0) {
        nearestcm = rgbmap->mapColor(r, g, b);
        /* rgb values for nearest color */
        nr = rgba_getr(palette->getEntry(nearestcm));
        ng = rgba_getg(palette->getEntry(nearestcm));
        nb = rgba_getb(palette->getEntry(nearestcm));
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
          oppr = rgba_getr(palette->getEntry(oppnrcm));
          oppg = rgba_getg(palette->getEntry(oppnrcm));
          oppb = rgba_getb(palette->getEntry(oppnrcm));

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

      *dst_it = nearestcm;
    }
  }

  return dst_image;
}

//////////////////////////////////////////////////////////////////////
// Creation of optimized palette for RGB images
// by David Capello

void PaletteOptimizer::feedWithImage(Image* image)
{
  uint32_t color;

  ASSERT(image);
  switch (image->pixelFormat()) {

    case IMAGE_RGB:
      {
        const LockImageBits<RgbTraits> bits(image);
        LockImageBits<RgbTraits>::const_iterator it = bits.begin(), end = bits.end();

        for (; it != end; ++it) {
          color = *it;

          if (rgba_geta(color) > 0) {
            color |= rgba(0, 0, 0, 255);
            m_histogram.addSamples(color, 1);
          }
        }
      }
      break;

    case IMAGE_GRAYSCALE:
      {
        const LockImageBits<RgbTraits> bits(image);
        LockImageBits<RgbTraits>::const_iterator it = bits.begin(), end = bits.end();

        for (; it != end; ++it) {
          color = *it;

          if (graya_geta(color) > 0) {
            color = graya_getv(color);
            m_histogram.addSamples(rgba(color, color, color, 255), 1);
          }
        }
      }
      break;

    case IMAGE_INDEXED:
      ASSERT(false);
      break;

  }
}

void PaletteOptimizer::calculate(Palette* palette, bool has_background_layer)
{
  // If the sprite has a background layer, the first entry can be
  // used, in other case the 0 indexed will be the mask color, so it
  // will not be used later in the color conversion (from RGB to
  // Indexed).
  int first_usable_entry = (has_background_layer ? 0: 1);
  //int used_colors =
  m_histogram.createOptimizedPalette(palette, first_usable_entry, 255);
  //palette->resize(first_usable_entry+used_colors);   // TODO
}

void create_palette_from_images(const std::vector<Image*>& images, Palette* palette, bool has_background_layer)
{
  PaletteOptimizer optimizer;
  for (int i=0; i<(int)images.size(); ++i)
    optimizer.feedWithImage(images[i]);

  optimizer.calculate(palette, has_background_layer);
}

} // namespace quantization
} // namespace doc
