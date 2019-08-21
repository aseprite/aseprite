// Aseprite Document Library
// Copyright (c) 2019 Igara Studio S.A.
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

#include <thread>

namespace doc {
namespace algorithm {

namespace {

template<typename ImageTraits>
bool is_same_pixel(color_t pixel1, color_t pixel2)
{
  static_assert(false && sizeof(ImageTraits), "No is_same_pixel impl");
  return false;
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

template<typename ImageTraits>
bool shrink_bounds_left_templ(const Image* image, gfx::Rect& bounds, color_t refpixel, int rowSize)
{
  int u, v;
  // Shrink left side
  for (u=bounds.x; u<bounds.x2(); ++u) {
    auto ptr = get_pixel_address_fast<ImageTraits>(image, u, v=bounds.y);
    for (; v<bounds.y2(); ++v, ptr+=rowSize) {
      ASSERT(ptr == get_pixel_address_fast<ImageTraits>(image, u, v));
      if (!is_same_pixel<ImageTraits>(*ptr, refpixel))
        return (!bounds.isEmpty());
    }
    ++bounds.x;
    --bounds.w;
  }
  return (!bounds.isEmpty());
}

template<typename ImageTraits>
bool shrink_bounds_right_templ(const Image* image, gfx::Rect& bounds, color_t refpixel, int rowSize)
{
  int u, v;
  // Shrink right side
  for (u=bounds.x2()-1; u>=bounds.x; --u) {
    auto ptr = get_pixel_address_fast<ImageTraits>(image, u, v=bounds.y);
    for (; v<bounds.y2(); ++v, ptr+=rowSize) {
      ASSERT(ptr == get_pixel_address_fast<ImageTraits>(image, u, v));
      if (!is_same_pixel<ImageTraits>(*ptr, refpixel))
        return (!bounds.isEmpty());
    }
    --bounds.w;
  }
  return (!bounds.isEmpty());
}

template<typename ImageTraits>
bool shrink_bounds_top_templ(const Image* image, gfx::Rect& bounds, color_t refpixel)
{
  int u, v;
  // Shrink top side
  for (v=bounds.y; v<bounds.y2(); ++v) {
    auto ptr = get_pixel_address_fast<ImageTraits>(image, u=bounds.x, v);
    for (; u<bounds.x2(); ++u, ++ptr) {
      ASSERT(ptr == get_pixel_address_fast<ImageTraits>(image, u, v));
      if (!is_same_pixel<ImageTraits>(*ptr, refpixel))
        return (!bounds.isEmpty());
    }
    ++bounds.y;
    --bounds.h;
  }
  return (!bounds.isEmpty());
}

template<typename ImageTraits>
bool shrink_bounds_bottom_templ(const Image* image, gfx::Rect& bounds, color_t refpixel)
{
  int u, v;
  // Shrink bottom side
  for (v=bounds.y2()-1; v>=bounds.y; --v) {
    auto ptr = get_pixel_address_fast<ImageTraits>(image, u=bounds.x, v);
    for (; u<bounds.x2(); ++u, ++ptr) {
      ASSERT(ptr == get_pixel_address_fast<ImageTraits>(image, u, v));
      if (!is_same_pixel<ImageTraits>(*ptr, refpixel))
        return (!bounds.isEmpty());
    }
    --bounds.h;
  }
  return (!bounds.isEmpty());
}

template<typename ImageTraits>
bool shrink_bounds_templ(const Image* image, gfx::Rect& bounds, color_t refpixel)
{
  // Pixels per row
  const int rowSize = image->getRowStrideSize() / image->getRowStrideSize(1);
  const int canvasSize = image->width()*image->height();
  if ((std::thread::hardware_concurrency() >= 4) &&
      ((image->pixelFormat() == IMAGE_RGB && canvasSize >= 800*800) ||
       (image->pixelFormat() != IMAGE_RGB && canvasSize >= 500*500))) {
    gfx::Rect
      leftBounds(bounds), rightBounds(bounds),
      topBounds(bounds), bottomBounds(bounds);
    std::thread left  ([&]{ shrink_bounds_left_templ  <ImageTraits>(image, leftBounds, refpixel, rowSize); });
    std::thread right ([&]{ shrink_bounds_right_templ <ImageTraits>(image, rightBounds, refpixel, rowSize); });
    std::thread top   ([&]{ shrink_bounds_top_templ   <ImageTraits>(image, topBounds, refpixel); });
    std::thread bottom([&]{ shrink_bounds_bottom_templ<ImageTraits>(image, bottomBounds, refpixel); });
    left.join();
    right.join();
    top.join();
    bottom.join();
    bounds = leftBounds;
    bounds &= rightBounds;
    bounds &= topBounds;
    bounds &= bottomBounds;
    return !bounds.isEmpty();
  }
  else {
    return
      shrink_bounds_left_templ<ImageTraits>(image, bounds, refpixel, rowSize) &&
      shrink_bounds_right_templ<ImageTraits>(image, bounds, refpixel, rowSize) &&
      shrink_bounds_top_templ<ImageTraits>(image, bounds, refpixel) &&
      shrink_bounds_bottom_templ<ImageTraits>(image, bounds, refpixel);
  }
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
    case IMAGE_BITMAP:
      // Not supported
      break;
  }
  ASSERT(false);
  bounds = start_bounds;
  return true;
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
