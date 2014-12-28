// Aseprite Gfx Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gfx/clip.h"

namespace gfx {

bool Clip::clip(
  int avail_dst_w,
  int avail_dst_h,
  int avail_src_w,
  int avail_src_h)
{
  // Clip srcBounds

  if (src.x < 0) {
    size.w += src.x;
    dst.x -= src.x;
    src.x = 0;
  }

  if (src.y < 0) {
    size.h += src.y;
    dst.y -= src.y;
    src.y = 0;
  }

  if (src.x + size.w > avail_src_w)
    size.w -= src.x + size.w - avail_src_w;

  if (src.y + size.h > avail_src_h)
    size.h -= src.y + size.h - avail_src_h;

  // Clip dstBounds

  if (dst.x < 0) {
    size.w += dst.x;
    src.x -= dst.x;
    dst.x = 0;
  }

  if (dst.y < 0) {
    size.h += dst.y;
    src.y -= dst.y;
    dst.y = 0;
  }

  if (dst.x + size.w > avail_dst_w)
    size.w -= dst.x + size.w - avail_dst_w;

  if (dst.y + size.h > avail_dst_h)
    size.h -= dst.y + size.h - avail_dst_h;

  return (size.w > 0 && size.h > 0);
}

} // namespace gfx
