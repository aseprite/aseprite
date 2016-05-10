// Aseprite Document Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/algorithm/shrink_bounds.h"

#include "doc/image.h"
#include "doc/image_impl.h"
#include "doc/primitives_fast.h"

namespace doc {
namespace algorithm {

namespace {

template<typename ImageTraits>
bool is_same_pixel(color_t pixel1, color_t pixel2)
{
  static_assert(false && sizeof(ImageTraits), "No is_same_pixel impl");
}

template<>
bool is_same_pixel<RgbTraits>(color_t pixel1, color_t pixel2)
{
  return (rgba_geta(pixel1) == 0 && rgba_geta(pixel2) == 0) || (pixel1 == pixel2);
}

template<>
bool is_same_pixel<GrayscaleTraits>(color_t pixel1, color_t pixel2)
{
  return (graya_geta(pixel1) == 0 && graya_geta(pixel2) == 0) || (pixel1 == pixel2);
}

template<>
bool is_same_pixel<IndexedTraits>(color_t pixel1, color_t pixel2)
{
  return pixel1 == pixel2;
}

template<>
bool is_same_pixel<BitmapTraits>(color_t pixel1, color_t pixel2)
{
  return pixel1 == pixel2;
}

template<typename ImageTraits>
bool shrink_bounds_templ(const Image* image, gfx::Rect& bounds, color_t refpixel)
{
  bool shrink;
  int u, v;

  // Shrink left side
  for (u=bounds.x; u<bounds.x+bounds.w; ++u) {
    shrink = true;
    for (v=bounds.y; v<bounds.y+bounds.h; ++v) {
      if (!is_same_pixel<ImageTraits>(
            get_pixel_fast<ImageTraits>(image, u, v), refpixel)) {
        shrink = false;
        break;
      }
    }
    if (!shrink)
      break;
    ++bounds.x;
    --bounds.w;
  }

  // Shrink right side
  for (u=bounds.x+bounds.w-1; u>=bounds.x; --u) {
    shrink = true;
    for (v=bounds.y; v<bounds.y+bounds.h; ++v) {
      if (!is_same_pixel<ImageTraits>(
            get_pixel_fast<ImageTraits>(image, u, v), refpixel)) {
        shrink = false;
        break;
      }
    }
    if (!shrink)
      break;
    --bounds.w;
  }

  // Shrink top side
  for (v=bounds.y; v<bounds.y+bounds.h; ++v) {
    shrink = true;
    for (u=bounds.x; u<bounds.x+bounds.w; ++u) {
      if (!is_same_pixel<ImageTraits>(
            get_pixel_fast<ImageTraits>(image, u, v), refpixel)) {
        shrink = false;
        break;
      }
    }
    if (!shrink)
      break;
    ++bounds.y;
    --bounds.h;
  }

  // Shrink bottom side
  for (v=bounds.y+bounds.h-1; v>=bounds.y; --v) {
    shrink = true;
    for (u=bounds.x; u<bounds.x+bounds.w; ++u) {
      if (!is_same_pixel<ImageTraits>(
            get_pixel_fast<ImageTraits>(image, u, v), refpixel)) {
        shrink = false;
        break;
      }
    }
    if (!shrink)
      break;
    --bounds.h;
  }

  return (!bounds.isEmpty());
}

template<typename ImageTraits>
bool shrink_bounds_templ2(const Image* a, const Image* b, gfx::Rect& bounds)
{
  bool shrink;
  int u, v;

  // Shrink left side
  for (u=bounds.x; u<bounds.x+bounds.w; ++u) {
    shrink = true;
    for (v=bounds.y; v<bounds.y+bounds.h; ++v) {
      if (get_pixel_fast<ImageTraits>(a, u, v) !=
          get_pixel_fast<ImageTraits>(b, u, v)) {
        shrink = false;
        break;
      }
    }
    if (!shrink)
      break;
    ++bounds.x;
    --bounds.w;
  }

  // Shrink right side
  for (u=bounds.x+bounds.w-1; u>=bounds.x; --u) {
    shrink = true;
    for (v=bounds.y; v<bounds.y+bounds.h; ++v) {
      if (get_pixel_fast<ImageTraits>(a, u, v) !=
          get_pixel_fast<ImageTraits>(b, u, v)) {
        shrink = false;
        break;
      }
    }
    if (!shrink)
      break;
    --bounds.w;
  }

  // Shrink top side
  for (v=bounds.y; v<bounds.y+bounds.h; ++v) {
    shrink = true;
    for (u=bounds.x; u<bounds.x+bounds.w; ++u) {
      if (get_pixel_fast<ImageTraits>(a, u, v) !=
          get_pixel_fast<ImageTraits>(b, u, v)) {
        shrink = false;
        break;
      }
    }
    if (!shrink)
      break;
    ++bounds.y;
    --bounds.h;
  }

  // Shrink bottom side
  for (v=bounds.y+bounds.h-1; v>=bounds.y; --v) {
    shrink = true;
    for (u=bounds.x; u<bounds.x+bounds.w; ++u) {
      if (get_pixel_fast<ImageTraits>(a, u, v) !=
          get_pixel_fast<ImageTraits>(b, u, v)) {
        shrink = false;
        break;
      }
    }
    if (!shrink)
      break;
    --bounds.h;
  }

  return (!bounds.isEmpty());
}

}

bool shrink_bounds(const Image* image,
                   const gfx::Rect& start_bounds,
                   gfx::Rect& bounds,
                   color_t refpixel)
{
  bounds = (start_bounds & image->bounds());
  switch (image->pixelFormat()) {
    case IMAGE_RGB:       return shrink_bounds_templ<RgbTraits>(image, bounds, refpixel);
    case IMAGE_GRAYSCALE: return shrink_bounds_templ<GrayscaleTraits>(image, bounds, refpixel);
    case IMAGE_INDEXED:   return shrink_bounds_templ<IndexedTraits>(image, bounds, refpixel);
    case IMAGE_BITMAP:    return shrink_bounds_templ<BitmapTraits>(image, bounds, refpixel);
  }
  ASSERT(false);
  return false;
}

bool shrink_bounds(const Image* image, gfx::Rect& bounds, color_t refpixel)
{
  return shrink_bounds(image, image->bounds(), bounds, refpixel);
}

bool shrink_bounds2(const Image* a, const Image* b,
                    const gfx::Rect& start_bounds,
                    gfx::Rect& bounds)
{
  ASSERT(a && b);
  ASSERT(a->bounds() == b->bounds());

  bounds = (start_bounds & a->bounds());

  switch (a->pixelFormat()) {
    case IMAGE_RGB:       return shrink_bounds_templ2<RgbTraits>(a, b, bounds);
    case IMAGE_GRAYSCALE: return shrink_bounds_templ2<GrayscaleTraits>(a, b, bounds);
    case IMAGE_INDEXED:   return shrink_bounds_templ2<IndexedTraits>(a, b, bounds);
    case IMAGE_BITMAP:    return shrink_bounds_templ2<BitmapTraits>(a, b, bounds);
  }
  ASSERT(false);
  return false;
}

} // namespace algorithm
} // namespace doc
