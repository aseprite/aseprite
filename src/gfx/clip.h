// Aseprite Gfx Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef GFX_CLIP_H_INCLUDED
#define GFX_CLIP_H_INCLUDED
#pragma once

#include "gfx/point.h"
#include "gfx/rect.h"
#include "gfx/size.h"

namespace gfx {

  class Clip {
  public:
    Point dst;
    Point src;
    Size size;

    Clip()
      : dst(0, 0)
      , src(0, 0)
      , size(0, 0) {
    }

    Clip(int w, int h)
      : dst(0, 0)
      , src(0, 0)
      , size(w, h) {
    }

    Clip(int dst_x, int dst_y, int src_x, int src_y, int w, int h)
      : dst(dst_x, dst_y)
      , src(src_x, src_y)
      , size(w, h) {
    }

    Clip(int dst_x, int dst_y, const Rect& srcBounds)
      : dst(dst_x, dst_y)
      , src(srcBounds.x, srcBounds.y)
      , size(srcBounds.w, srcBounds.h) {
    }

    Clip(const Point& dst, const Point& src, const Size& size)
      : dst(dst)
      , src(src)
      , size(size) {
    }

    Clip(const Point& dst, const Rect& srcBounds)
      : dst(dst)
      , src(srcBounds.x, srcBounds.y)
      , size(srcBounds.w, srcBounds.h) {
    }

    Clip(const Rect& bounds)
      : dst(bounds.x, bounds.y)
      , src(bounds.x, bounds.y)
      , size(bounds.w, bounds.h) {
    }

    Rect dstBounds() const { return Rect(dst, size); }
    Rect srcBounds() const { return Rect(src, size); }

    bool operator==(const Clip& other) const {
      return (dst == other.dst &&
        src == other.src &&
        size == other.size);
    }

    bool clip(
      // Available area
      int avail_dst_w,
      int avail_dst_h,
      int avail_src_w,
      int avail_src_h);

  };

} // namespace gfx

#endif
