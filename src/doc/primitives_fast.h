// Aseprite Document Library
// Copyright (c) 2023 Igara Studio S.A.
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_PRIMITIVES_FAST_H_INCLUDED
#define DOC_PRIMITIVES_FAST_H_INCLUDED
#pragma once

#include "doc/color.h"
#include "doc/image_traits.h"

namespace doc {
class Image;
template<typename ImageTraits>
class ImageImpl;

template<class Traits>
inline typename Traits::address_t get_pixel_address_fast(const Image* image, int x, int y)
{
  ASSERT(x >= 0 && x < image->width());
  ASSERT(y >= 0 && y < image->height());

  return (((ImageImpl<Traits>*)image)->address(x, y));
}

template<class Traits>
inline typename Traits::pixel_t get_pixel_fast(const Image* image, int x, int y)
{
  ASSERT(x >= 0 && x < image->width());
  ASSERT(y >= 0 && y < image->height());

  return *(((ImageImpl<Traits>*)image)->address(x, y));
}

template<class Traits>
inline void put_pixel_fast(Image* image, int x, int y, typename Traits::pixel_t color)
{
  ASSERT(x >= 0 && x < image->width());
  ASSERT(y >= 0 && y < image->height());

  *(((ImageImpl<Traits>*)image)->address(x, y)) = color;
}

//////////////////////////////////////////////////////////////////////
// Bitmap specialization

template<>
inline BitmapTraits::pixel_t get_pixel_fast<BitmapTraits>(const Image* image, int x, int y)
{
  ASSERT(x >= 0 && x < image->width());
  ASSERT(y >= 0 && y < image->height());

  return (*image->getPixelAddress(x, y)) & (1 << (x % 8)) ? 1 : 0;
}

template<>
inline void put_pixel_fast<BitmapTraits>(Image* image, int x, int y, BitmapTraits::pixel_t color)
{
  ASSERT(x >= 0 && x < image->width());
  ASSERT(y >= 0 && y < image->height());

  if (color)
    *image->getPixelAddress(x, y) |= (1 << (x % 8));
  else
    *image->getPixelAddress(x, y) &= ~(1 << (x % 8));
}

} // namespace doc

#endif
