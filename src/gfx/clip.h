// Aseprite Gfx Library
// Copyright (c) 2001-2016 David Capello
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

  template<typename T>
  class ClipT {
  public:
    PointT<T> dst;
    PointT<T> src;
    SizeT<T> size;

    ClipT()
      : dst(0, 0)
      , src(0, 0)
      , size(0, 0) {
    }

    ClipT(T w, T h)
      : dst(0, 0)
      , src(0, 0)
      , size(w, h) {
    }

    ClipT(T dst_x, T dst_y, T src_x, T src_y, T w, T h)
      : dst(dst_x, dst_y)
      , src(src_x, src_y)
      , size(w, h) {
    }

    ClipT(T dst_x, T dst_y, const RectT<T>& srcBounds)
      : dst(dst_x, dst_y)
      , src(srcBounds.x, srcBounds.y)
      , size(srcBounds.w, srcBounds.h) {
    }

    ClipT(const PointT<T>& dst, const PointT<T>& src, const SizeT<T>& size)
      : dst(dst)
      , src(src)
      , size(size) {
    }

    ClipT(const PointT<T>& dst, const RectT<T>& srcBounds)
      : dst(dst)
      , src(srcBounds.x, srcBounds.y)
      , size(srcBounds.w, srcBounds.h) {
    }

    ClipT(const RectT<T>& bounds)
      : dst(bounds.x, bounds.y)
      , src(bounds.x, bounds.y)
      , size(bounds.w, bounds.h) {
    }

    RectT<T> dstBounds() const { return RectT<T>(dst, size); }
    RectT<T> srcBounds() const { return RectT<T>(src, size); }

    bool operator==(const ClipT<T>& other) const {
      return (dst == other.dst &&
        src == other.src &&
        size == other.size);
    }

    bool clip(
      // Available area
      T avail_dst_w,
      T avail_dst_h,
      T avail_src_w,
      T avail_src_h) {
      // Clip srcBounds

      if (src.x < T(0)) {
        size.w += src.x;
        dst.x -= src.x;
        src.x = T(0);
      }

      if (src.y < T(0)) {
        size.h += src.y;
        dst.y -= src.y;
        src.y = T(0);
      }

      if (src.x + size.w > avail_src_w)
        size.w -= src.x + size.w - avail_src_w;

      if (src.y + size.h > avail_src_h)
        size.h -= src.y + size.h - avail_src_h;

      // Clip dstBounds

      if (dst.x < T(0)) {
        size.w += dst.x;
        src.x -= dst.x;
        dst.x = T(0);
      }

      if (dst.y < T(0)) {
        size.h += dst.y;
        src.y -= dst.y;
        dst.y = T(0);
      }

      if (dst.x + size.w > avail_dst_w)
        size.w -= dst.x + size.w - avail_dst_w;

      if (dst.y + size.h > avail_dst_h)
        size.h -= dst.y + size.h - avail_dst_h;

      return (size.w > T(0) && size.h > T(0));
    }

  };

  typedef ClipT<int> Clip;
  typedef ClipT<double> ClipF;

} // namespace gfx

#endif
