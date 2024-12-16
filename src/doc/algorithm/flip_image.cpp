// Aseprite Document Library
// Copyright (c) 2023 Igara Studio S.A.
// Copyright (c) 2001-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/algorithm/flip_image.h"

#include "doc/dispatch.h"
#include "doc/image.h"
#include "doc/image_impl.h"
#include "doc/mask.h"
#include "doc/primitives.h"
#include "doc/primitives_fast.h"
#include "gfx/rect.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

namespace doc { namespace algorithm {

template<typename ImageTraits>
void flip_image_with_put_pixel_fast_templ(Image* image, const gfx::Rect& bounds, FlipType flipType)
{
  switch (flipType) {
    case FlipHorizontal:
      for (int y = bounds.y; y < bounds.y2(); ++y) {
        int u = bounds.x2() - 1;
        for (int x = bounds.x; x < bounds.x + bounds.w / 2; ++x, --u) {
          uint32_t c1 = get_pixel_fast<ImageTraits>(image, x, y);
          uint32_t c2 = get_pixel_fast<ImageTraits>(image, u, y);
          put_pixel_fast<ImageTraits>(image, x, y, c2);
          put_pixel_fast<ImageTraits>(image, u, y, c1);
        }
      }
      break;

    case FlipVertical: {
      int v = bounds.y2() - 1;
      for (int y = bounds.y; y < bounds.y + bounds.h / 2; ++y, --v) {
        for (int x = bounds.x; x < bounds.x2(); ++x) {
          uint32_t c1 = get_pixel_fast<ImageTraits>(image, x, y);
          uint32_t c2 = get_pixel_fast<ImageTraits>(image, x, v);
          put_pixel_fast<ImageTraits>(image, x, y, c2);
          put_pixel_fast<ImageTraits>(image, x, v, c1);
        }
      }
      break;
    }

    case FlipDiagonal: {
      int d = std::min(bounds.w, bounds.h);
      for (int y = bounds.y; y < bounds.y + d; ++y) {
        for (int x = bounds.x + y; x < bounds.x + d; ++x) {
          uint32_t c1 = get_pixel_fast<ImageTraits>(image, x, y);
          uint32_t c2 = get_pixel_fast<ImageTraits>(image, y, x);
          put_pixel_fast<ImageTraits>(image, x, y, c2);
          put_pixel_fast<ImageTraits>(image, y, x, c1);
        }
      }
      break;
    }
  }
}

template<typename ImageTraits>
void flip_image_with_rawptr_templ(Image* image, const gfx::Rect& bounds, FlipType flipType)
{
  using address_t = typename ImageTraits::address_t;

  switch (flipType) {
    case FlipHorizontal:
      for (int y = bounds.y; y < bounds.y2(); ++y) {
        const int n = bounds.w / 2;
        auto l = (address_t)image->getPixelAddress(bounds.x, y);
        auto r = (address_t)image->getPixelAddress(bounds.x2() - 1, y);

        for (int x = 0; x < n; ++x, ++l, --r) {
          std::swap(*l, *r);
        }
      }
      break;

    case FlipVertical: {
      const int n = bounds.w;
      int v = bounds.y2() - 1;
      for (int y = bounds.y; y < bounds.y + bounds.h / 2; ++y, --v) {
        auto t = (address_t)image->getPixelAddress(bounds.x, y);
        auto b = (address_t)image->getPixelAddress(bounds.x, v);

        for (int x = 0; x < n; ++x, ++t, ++b) {
          std::swap(*t, *b);
        }
      }
      break;
    }

    case FlipDiagonal:
      flip_image_with_put_pixel_fast_templ<ImageTraits>(image, bounds, flipType);
      break;
  }
}

void flip_image_slow(Image* image, const gfx::Rect& bounds, FlipType flipType)
{
  DOC_DISPATCH_BY_COLOR_MODE(image->colorMode(),
                             flip_image_with_put_pixel_fast_templ,
                             image,
                             bounds,
                             flipType);
}

void flip_image(Image* image, const gfx::Rect& bounds, FlipType flipType)
{
  // Use get/put_pixel_fast for IMAGE_BITMAP as we cannot use the
  // rawptr to iterate through bits.
  if (image->colorMode() == ColorMode::BITMAP) {
    return flip_image_with_put_pixel_fast_templ<BitmapTraits>(image, bounds, flipType);
  }

  DOC_DISPATCH_BY_COLOR_MODE_EXCLUDE_BITMAP(image->colorMode(),
                                            flip_image_with_rawptr_templ,
                                            image,
                                            bounds,
                                            flipType);
}

template<typename ImageTraits>
void flip_image_with_mask_templ(Image* image, const Mask* mask, FlipType flipType, int bgcolor)
{
  gfx::Rect bounds = mask->bounds();

  switch (flipType) {
    case FlipHorizontal: {
      std::unique_ptr<Image> originalRow(Image::create(image->pixelFormat(), bounds.w, 1));

      for (int y = bounds.y; y < bounds.y2(); ++y) {
        // Copy the current row.
        originalRow->copy(image, gfx::Clip(0, 0, bounds.x, y, bounds.w, 1));

        int u = bounds.x2() - 1;
        for (int x = bounds.x; x < bounds.x2(); ++x, --u) {
          if (mask->containsPoint(x, y)) {
            put_pixel_fast<ImageTraits>(
              image,
              u,
              y,
              get_pixel_fast<ImageTraits>(originalRow.get(), x - bounds.x, 0));

            if (!mask->containsPoint(u, y))
              put_pixel_fast<ImageTraits>(image, x, y, bgcolor);
          }
        }
      }
      break;
    }

    case FlipVertical: {
      std::unique_ptr<Image> originalCol(Image::create(image->pixelFormat(), 1, bounds.h));

      for (int x = bounds.x; x < bounds.x2(); ++x) {
        // Copy the current column.
        originalCol->copy(image, gfx::Clip(0, 0, x, bounds.y, 1, bounds.h));

        int v = bounds.y2() - 1;
        for (int y = bounds.y; y < bounds.y2(); ++y, --v) {
          if (mask->containsPoint(x, y)) {
            put_pixel_fast<ImageTraits>(
              image,
              x,
              v,
              get_pixel_fast<ImageTraits>(originalCol.get(), 0, y - bounds.y));

            if (!mask->containsPoint(x, v))
              put_pixel_fast<ImageTraits>(image, x, y, bgcolor);
          }
        }
      }
      break;
    }

    // TODO
    case FlipDiagonal: ASSERT(false); break;
  }
}

void flip_image_with_mask(Image* image, const Mask* mask, FlipType flipType, int bgcolor)
{
  DOC_DISPATCH_BY_COLOR_MODE(image->colorMode(),
                             flip_image_with_mask_templ,
                             image,
                             mask,
                             flipType,
                             bgcolor);
}

}} // namespace doc::algorithm
