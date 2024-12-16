// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/image_impl.h"

#include "doc/image_iterator.h"
#include "doc/image_traits.h"

namespace doc {

void copy_bitmaps(Image* dst, const Image* src, gfx::Clip area)
{
  if (!area.clip(dst->width(), dst->height(), src->width(), src->height()))
    return;

  // Copy process
  ImageConstIterator<BitmapTraits> src_it(src, area.srcBounds(), area.src.x, area.src.y);
  ImageIterator<BitmapTraits> dst_it(dst, area.dstBounds(), area.dst.x, area.dst.y);

  int end_x = area.dst.x + area.size.w;

  for (int end_y = area.dst.y + area.size.h; area.dst.y < end_y; ++area.dst.y, ++area.src.y) {
    for (int x = area.dst.x; x < end_x; ++x) {
      *dst_it = *src_it;
      ++src_it;
      ++dst_it;
    }
  }
}

} // namespace doc
