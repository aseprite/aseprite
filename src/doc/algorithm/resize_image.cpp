// Aseprite Document Library
// Copyright (c) 2019  Igara Studio S.A.
// Copyright (c) 2001-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/algorithm/resize_image.h"

#include "doc/algorithm/rotsprite.h"
#include "doc/image_impl.h"
#include "doc/palette.h"
#include "doc/primitives_fast.h"
#include "doc/rgbmap.h"
#include "gfx/point.h"

#include <cmath>

namespace doc {
namespace algorithm {

template<typename ImageTraits>
void resize_image_nearest(const Image* src, Image* dst)
{
  double x_ratio = double(src->width()) / double(dst->width());
  double y_ratio = double(src->height()) / double(dst->height());
  double px, py;

  LockImageBits<ImageTraits> dstBits(dst);
  auto dstIt = dstBits.begin();

  for (int y=0; y<dst->height(); ++y) {
    py = std::floor(y * y_ratio);
    for (int x=0; x<dst->width(); ++x, ++dstIt) {
      px = std::floor(x * x_ratio);
      *dstIt = get_pixel_fast<ImageTraits>(src, int(px), int(py));
    }
  }
}

void resize_image(const Image* src,
                  Image* dst,
                  const ResizeMethod method,
                  const Palette* pal,
                  const RgbMap* rgbmap,
                  const color_t maskColor)
{
  switch (method) {

    // TODO optimize this
    case RESIZE_METHOD_NEAREST_NEIGHBOR: {
      ASSERT(src->pixelFormat() == dst->pixelFormat());

      switch (src->pixelFormat()) {
        case IMAGE_RGB: resize_image_nearest<RgbTraits>(src, dst); break;
        case IMAGE_GRAYSCALE: resize_image_nearest<GrayscaleTraits>(src, dst); break;
        case IMAGE_INDEXED: resize_image_nearest<IndexedTraits>(src, dst); break;
        case IMAGE_BITMAP: resize_image_nearest<BitmapTraits>(src, dst); break;
      }
      break;
    }

    // TODO optimize this
    case RESIZE_METHOD_BILINEAR: {
      uint32_t color[4], dst_color = 0;
      double u, v, du, dv;
      int u_floor, u_floor2;
      int v_floor, v_floor2;
      int x, y;

      // We cannot do interpolations between RGB values on indexed
      // images without a palette/rgbmap.
      if (dst->pixelFormat() == IMAGE_INDEXED &&
          (!pal || !rgbmap)) {
        resize_image(
          src, dst,
          RESIZE_METHOD_NEAREST_NEIGHBOR,
          pal, rgbmap, maskColor);
        return;
      }

      u = v = 0.0;
      du = (src->width()-1) * 1.0 / (dst->width()-1);
      dv = (src->height()-1) * 1.0 / (dst->height()-1);
      for (y=0; y<dst->height(); ++y) {
        for (x=0; x<dst->width(); ++x) {
          u_floor = (int)std::floor(u);
          v_floor = (int)std::floor(v);

          if (u_floor > src->width()-1) {
            u_floor = src->width()-1;
            u_floor2 = src->width()-1;
          }
          else if (u_floor == src->width()-1)
            u_floor2 = u_floor;
          else
            u_floor2 = u_floor+1;

          if (v_floor > src->height()-1) {
            v_floor = src->height()-1;
            v_floor2 = src->height()-1;
          }
          else if (v_floor == src->height()-1)
            v_floor2 = v_floor;
          else
            v_floor2 = v_floor+1;

          // get the four colors
          color[0] = src->getPixel(u_floor,  v_floor);
          color[1] = src->getPixel(u_floor2, v_floor);
          color[2] = src->getPixel(u_floor,  v_floor2);
          color[3] = src->getPixel(u_floor2, v_floor2);

          // calculate the interpolated color
          double u1 = u - u_floor;
          double v1 = v - v_floor;
          double u2 = 1 - u1;
          double v2 = 1 - v1;

          switch (dst->pixelFormat()) {
            case IMAGE_RGB: {
              int r = int((rgba_getr(color[0])*u2 + rgba_getr(color[1])*u1)*v2 +
                          (rgba_getr(color[2])*u2 + rgba_getr(color[3])*u1)*v1);
              int g = int((rgba_getg(color[0])*u2 + rgba_getg(color[1])*u1)*v2 +
                          (rgba_getg(color[2])*u2 + rgba_getg(color[3])*u1)*v1);
              int b = int((rgba_getb(color[0])*u2 + rgba_getb(color[1])*u1)*v2 +
                          (rgba_getb(color[2])*u2 + rgba_getb(color[3])*u1)*v1);
              int a = int((rgba_geta(color[0])*u2 + rgba_geta(color[1])*u1)*v2 +
                          (rgba_geta(color[2])*u2 + rgba_geta(color[3])*u1)*v1);
              dst_color = rgba(r, g, b, a);
              break;
            }
            case IMAGE_GRAYSCALE: {
              int v = int((graya_getv(color[0])*u2 + graya_getv(color[1])*u1)*v2 +
                          (graya_getv(color[2])*u2 + graya_getv(color[3])*u1)*v1);
              int a = int((graya_geta(color[0])*u2 + graya_geta(color[1])*u1)*v2 +
                          (graya_geta(color[2])*u2 + graya_geta(color[3])*u1)*v1);
              dst_color = graya(v, a);
              break;
            }
            case IMAGE_INDEXED: {
              // Convert index to RGBA values
              for (int i=0; i<4; ++i) {
                if (color[i] == maskColor)
                  color[i] = pal->getEntry(color[i]) & rgba_rgb_mask; // Set alpha = 0
                else
                  color[i] = pal->getEntry(color[i]);
              }

              int r = int((rgba_getr(color[0])*u2 + rgba_getr(color[1])*u1)*v2 +
                          (rgba_getr(color[2])*u2 + rgba_getr(color[3])*u1)*v1);
              int g = int((rgba_getg(color[0])*u2 + rgba_getg(color[1])*u1)*v2 +
                          (rgba_getg(color[2])*u2 + rgba_getg(color[3])*u1)*v1);
              int b = int((rgba_getb(color[0])*u2 + rgba_getb(color[1])*u1)*v2 +
                          (rgba_getb(color[2])*u2 + rgba_getb(color[3])*u1)*v1);
              int a = int((rgba_geta(color[0])*u2 + rgba_geta(color[1])*u1)*v2 +
                          (rgba_geta(color[2])*u2 + rgba_geta(color[3])*u1)*v1);
              dst_color = rgbmap->mapColor(r, g, b, a);
              break;
            }
          }

          dst->putPixel(x, y, dst_color);
          u += du;
        }
        u = 0.0;
        v += dv;
      }
      break;
    }

    case RESIZE_METHOD_ROTSPRITE: {
      rotsprite_image(
        dst, src, nullptr,
        0, 0,
        dst->width(), 0,
        dst->width(), dst->height(),
        0, dst->height());
      break;
    }

  }
}

void fixup_image_transparent_colors(Image* image)
{
  int x, y;

  switch (image->pixelFormat()) {

    case IMAGE_RGB: {
      int r, g, b, count;
      LockImageBits<RgbTraits> bits(image);
      LockImageBits<RgbTraits>::iterator it = bits.begin();

      for (y=0; y<image->height(); ++y) {
        for (x=0; x<image->width(); ++x, ++it) {
          uint32_t c = *it;

          // if this is a completely-transparent pixel...
          if (rgba_geta(c) == 0) {
            count = 0;
            r = g = b = 0;

            gfx::Rect area = gfx::Rect(x-1, y-1, 3, 3).createIntersection(image->bounds());
            LockImageBits<RgbTraits>::iterator it2 = bits.begin_area(area);
            LockImageBits<RgbTraits>::iterator end2 = bits.end_area(area);

            for (; it2 != end2; ++it2) {
              c = *it2;
              if (rgba_geta(c) > 0) {
                r += rgba_getr(c);
                g += rgba_getg(c);
                b += rgba_getb(c);
                ++count;
              }
            }

            if (count > 0) {
              r /= count;
              g /= count;
              b /= count;
              *it = rgba(r, g, b, 0);
            }
          }
        }
      }
      break;
    }

    case IMAGE_GRAYSCALE: {
      int k, count;
      LockImageBits<GrayscaleTraits> bits(image);
      LockImageBits<GrayscaleTraits>::iterator it = bits.begin();

      for (y=0; y<image->height(); ++y) {
        for (x=0; x<image->width(); ++x, ++it) {
          uint16_t c = *it;

          // If this is a completely-transparent pixel...
          if (graya_geta(c) == 0) {
            count = 0;
            k = 0;

            gfx::Rect area = gfx::Rect(x-1, y-1, 3, 3).createIntersection(image->bounds());
            LockImageBits<GrayscaleTraits>::iterator it2 = bits.begin_area(area);
            LockImageBits<GrayscaleTraits>::iterator end2 = bits.end_area(area);

            for (; it2 != end2; ++it2) {
              c = *it2;
              if (graya_geta(c) > 0) {
                k += graya_getv(c);
                ++count;
              }
            }

            if (count > 0) {
              k /= count;
              *it = graya(k, 0);
            }
          }
        }
      }
      break;
    }

  }
}

} // namespace algorithm
} // namespace doc
