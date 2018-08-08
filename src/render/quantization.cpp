// Aseprite Render Library
// Copyright (c) 2001-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "render/quantization.h"

#include "doc/image_impl.h"
#include "doc/layer.h"
#include "doc/palette.h"
#include "doc/primitives.h"
#include "doc/remap.h"
#include "doc/rgbmap.h"
#include "doc/sprite.h"
#include "gfx/hsv.h"
#include "gfx/rgb.h"
#include "render/ordered_dither.h"
#include "render/render.h"
#include "render/task_delegate.h"

#include <algorithm>
#include <limits>
#include <map>
#include <memory>
#include <vector>

namespace render {

using namespace doc;
using namespace gfx;

Palette* create_palette_from_sprite(
  const Sprite* sprite,
  const frame_t fromFrame,
  const frame_t toFrame,
  const bool withAlpha,
  Palette* palette,
  TaskDelegate* delegate)
{
  PaletteOptimizer optimizer;

  if (!palette)
    palette = new Palette(fromFrame, 256);

  // Add a flat image with the current sprite's frame rendered
  ImageRef flat_image(Image::create(IMAGE_RGB,
      sprite->width(), sprite->height()));

  // Feed the optimizer with all rendered frames
  render::Render render;
  for (frame_t frame=fromFrame; frame<=toFrame; ++frame) {
    render.renderSprite(flat_image.get(), sprite, frame);
    optimizer.feedWithImage(flat_image.get(), withAlpha);

    if (delegate) {
      if (!delegate->continueTask())
        return nullptr;

      delegate->notifyTaskProgress(
        double(frame-fromFrame+1) / double(toFrame-fromFrame+1));
    }
  }

  // Generate an optimized palette
  optimizer.calculate(
    palette,
    // Transparent color is needed if we have transparent layers
    (sprite->backgroundLayer() &&
     sprite->allLayersCount() == 1 ? -1: sprite->transparentColor()));

  return palette;
}

Image* convert_pixel_format(
  const Image* image,
  Image* new_image,
  PixelFormat pixelFormat,
  DitheringAlgorithm ditheringAlgorithm,
  const DitheringMatrix& ditheringMatrix,
  const RgbMap* rgbmap,
  const Palette* palette,
  bool is_background,
  color_t new_mask_color,
  TaskDelegate* delegate)
{
  if (!new_image)
    new_image = Image::create(pixelFormat, image->width(), image->height());
  new_image->setMaskColor(new_mask_color);

  // RGB -> Indexed with ordered dithering
  if (image->pixelFormat() == IMAGE_RGB &&
      pixelFormat == IMAGE_INDEXED &&
      ditheringAlgorithm != DitheringAlgorithm::None) {
    std::unique_ptr<DitheringAlgorithmBase> dither;
    switch (ditheringAlgorithm) {
      case DitheringAlgorithm::Ordered:
        dither.reset(new OrderedDither2(is_background ? -1: new_mask_color));
        break;
      case DitheringAlgorithm::Old:
        dither.reset(new OrderedDither(is_background ? -1: new_mask_color));
        break;
    }
    if (dither)
      dither_rgb_image_to_indexed(
        *dither, ditheringMatrix, image, new_image, 0, 0, rgbmap, palette, delegate);
    return new_image;
  }

  color_t c;
  int r, g, b, a;

  switch (image->pixelFormat()) {

    case IMAGE_RGB: {
      const LockImageBits<RgbTraits> srcBits(image);
      LockImageBits<RgbTraits>::const_iterator src_it = srcBits.begin(), src_end = srcBits.end();

      switch (new_image->pixelFormat()) {

        // RGB -> RGB
        case IMAGE_RGB:
          new_image->copy(image, gfx::Clip(image->bounds()));
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
            a = rgba_geta(c);

            if (a == 0)
              *dst_it = new_mask_color;
            else if (rgbmap)
              *dst_it = rgbmap->mapColor(r, g, b, a);
            else
              *dst_it = palette->findBestfit(r, g, b, a, new_mask_color);
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
          new_image->copy(image, gfx::Clip(image->bounds()));
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
            a = graya_geta(c);
            c = graya_getv(c);

            if (a == 0)
              *dst_it = new_mask_color;
            else if (rgbmap)
              *dst_it = rgbmap->mapColor(c, c, c, a);
            else
              *dst_it = palette->findBestfit(c, c, c, a, new_mask_color);
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
              *dst_it = rgba(0, 0, 0, 0);
            else
              *dst_it = palette->getEntry(c);
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
              *dst_it = graya(0, 0);
            else {
              c = palette->getEntry(c);
              r = rgba_getr(c);
              g = rgba_getg(c);
              b = rgba_getb(c);
              a = rgba_geta(c);

              g = 255 * Hsv(Rgb(r, g, b)).valueInt() / 100;
              *dst_it = graya(g, a);
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

          for (; src_it != src_end; ++src_it, ++dst_it) {
            ASSERT(dst_it != dst_end);
            c = *src_it;

            if (!is_background && c == image->maskColor())
              *dst_it = new_mask_color;
            else {
              c = palette->getEntry(c);
              r = rgba_getr(c);
              g = rgba_getg(c);
              b = rgba_getb(c);
              a = rgba_geta(c);

              if (rgbmap)
                *dst_it = rgbmap->mapColor(r, g, b, a);
              else
                *dst_it = palette->findBestfit(r, g, b, a, new_mask_color);
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

//////////////////////////////////////////////////////////////////////
// Creation of optimized palette for RGB images
// by David Capello

void PaletteOptimizer::feedWithImage(Image* image, bool withAlpha)
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
            if (!withAlpha)
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
            if (!withAlpha)
              color = graya(graya_getv(color), 255);

            m_histogram.addSamples(rgba(graya_getv(color),
                                        graya_getv(color),
                                        graya_getv(color),
                                        graya_geta(color)), 1);
          }
        }
      }
      break;

    case IMAGE_INDEXED:
      ASSERT(false);
      break;

  }
}

void PaletteOptimizer::feedWithRgbaColor(color_t color)
{
  m_histogram.addSamples(color, 1);
}

void PaletteOptimizer::calculate(Palette* palette, int maskIndex)
{
  bool addMask;

  if ((palette->size() > 1) &&
      (maskIndex >= 0 && maskIndex < palette->size())) {
    palette->resize(palette->size()-1);
    addMask = true;
  }
  else
    addMask = false;

  // If the sprite has a background layer, the first entry can be
  // used, in other case the 0 indexed will be the mask color, so it
  // will not be used later in the color conversion (from RGB to
  // Indexed).
  int usedColors = m_histogram.createOptimizedPalette(palette);

  if (addMask) {
    palette->resize(usedColors+1);

    Remap remap(palette->size());
    for (int i=0; i<usedColors; ++i)
      remap.map(i, i + (i >= maskIndex ? 1: 0));

    palette->applyRemap(remap);

    if (maskIndex < palette->size())
      palette->setEntry(maskIndex, rgba(0, 0, 0, 255));
  }
  else
    palette->resize(MAX(1, usedColors));
}

} // namespace render
