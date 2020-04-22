// Aseprite Document Library
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/mask.h"

#include "base/clamp.h"
#include "base/memory.h"
#include "doc/image_impl.h"

#include <cstdlib>
#include <cstring>

namespace doc {

namespace {

  template<typename Func>
  void for_each_mask_pixel(Mask& a, const Mask& b, Func f) {
    a.reserve(b.bounds());

    {
      LockImageBits<BitmapTraits> aBits(a.bitmap());
      auto aIt = aBits.begin();

      auto bounds = a.bounds();
      for (int y=0; y<bounds.h; ++y) {
        for (int x=0; x<bounds.w; ++x, ++aIt) {
          color_t aColor = *aIt;
          color_t bColor = (b.containsPoint(bounds.x+x,
                                            bounds.y+y) ? 1: 0);
          *aIt = f(aColor, bColor);
        }
      }
    }

    a.shrink();
  }

} // namespace namespace

Mask::Mask()
  : Object(ObjectType::Mask)
{
  initialize();
}

Mask::Mask(const Mask& mask)
  : Object(mask)
{
  initialize();
  copyFrom(&mask);
}

Mask::~Mask()
{
  ASSERT(m_freeze_count == 0);
}

void Mask::initialize()
{
  m_freeze_count = 0;
  m_bounds = gfx::Rect(0, 0, 0, 0);
}

int Mask::getMemSize() const
{
  return sizeof(Mask) + (m_bitmap ? m_bitmap->getMemSize(): 0);
}

void Mask::setName(const char *name)
{
  m_name = name;
}

void Mask::freeze()
{
  ASSERT(m_freeze_count >= 0);
  m_freeze_count++;
}

void Mask::unfreeze()
{
  ASSERT(m_freeze_count > 0);
  m_freeze_count--;

  // Shrink just in case
  if (m_freeze_count == 0)
    shrink();
}

bool Mask::isRectangular() const
{
  if (!m_bitmap)
    return false;

  LockImageBits<BitmapTraits> bits(m_bitmap.get());
  LockImageBits<BitmapTraits>::iterator it = bits.begin(), end = bits.end();

  for (; it != end; ++it) {
    if (*it == 0)
      return false;
  }

  return true;
}

void Mask::copyFrom(const Mask* sourceMask)
{
  ASSERT(m_freeze_count == 0);

  clear();
  setName(sourceMask->name().c_str());

  if (sourceMask->bitmap()) {
    // Add all the area of "mask"
    add(sourceMask->bounds());

    // And copy the "mask" bitmap (m_bitmap can be nullptr if this is
    // frozen, so add() doesn't created the bitmap)
    if (m_bitmap)
      copy_image(m_bitmap.get(), sourceMask->m_bitmap.get());
  }
}

void Mask::offsetOrigin(int dx, int dy)
{
  m_bounds.offset(dx, dy);
}

void Mask::clear()
{
  m_bitmap.reset();
  m_bounds = gfx::Rect(0, 0, 0, 0);
}

void Mask::invert()
{
  if (!m_bitmap)
    return;

  LockImageBits<BitmapTraits> bits(m_bitmap.get());
  LockImageBits<BitmapTraits>::iterator it = bits.begin(), end = bits.end();

  for (; it != end; ++it)
    *it = (*it ? 0: 1);

  shrink();
}

void Mask::replace(const gfx::Rect& bounds)
{
  m_bounds = bounds;

  m_bitmap.reset(Image::create(IMAGE_BITMAP, bounds.w, bounds.h, m_buffer));
  clear_image(m_bitmap.get(), 1);
}

void Mask::add(const doc::Mask& mask)
{
  for_each_mask_pixel(
    *this, mask,
    [](color_t a, color_t b) -> color_t {
      return a | b;
    });
}

void Mask::subtract(const doc::Mask& mask)
{
  for_each_mask_pixel(
    *this, mask,
    [](color_t a, color_t b) -> color_t {
      if (a)
        return a - b;
      else
        return 0;
    });
}

void Mask::intersect(const doc::Mask& mask)
{
  for_each_mask_pixel(
    *this, mask,
    [](color_t a, color_t b) -> color_t {
      return a & b;
    });
}

void Mask::add(const gfx::Rect& bounds)
{
  if (m_freeze_count == 0)
    reserve(bounds);

  // m_bitmap can be nullptr if we have m_freeze_count > 0
  if (!m_bitmap)
    return;

  fill_rect(m_bitmap.get(),
            bounds.x-m_bounds.x,
            bounds.y-m_bounds.y,
            bounds.x-m_bounds.x+bounds.w-1,
            bounds.y-m_bounds.y+bounds.h-1, 1);
}

void Mask::subtract(const gfx::Rect& bounds)
{
  if (!m_bitmap)
    return;

  fill_rect(m_bitmap.get(),
    bounds.x-m_bounds.x,
    bounds.y-m_bounds.y,
    bounds.x-m_bounds.x+bounds.w-1,
    bounds.y-m_bounds.y+bounds.h-1, 0);

  shrink();
}

void Mask::intersect(const gfx::Rect& bounds)
{
  if (!m_bitmap)
    return;

  gfx::Rect newBounds = m_bounds.createIntersection(bounds);

  Image* image = NULL;

  if (!newBounds.isEmpty()) {
    image = crop_image(
      m_bitmap.get(),
      newBounds.x-m_bounds.x,
      newBounds.y-m_bounds.y,
      newBounds.w,
      newBounds.h, 0);
  }

  m_bitmap.reset(image);
  m_bounds = newBounds;

  shrink();
}

void Mask::byColor(const Image *src, int color, int fuzziness)
{
  replace(src->bounds());

  Image* dst = m_bitmap.get();

  switch (src->pixelFormat()) {

    case IMAGE_RGB: {
      const LockImageBits<RgbTraits> srcBits(src);
      LockImageBits<BitmapTraits> dstBits(dst, Image::WriteLock);
      LockImageBits<RgbTraits>::const_iterator src_it = srcBits.begin(), src_end = srcBits.end();
      LockImageBits<BitmapTraits>::iterator dst_it = dstBits.begin();
#ifdef _DEBUG
      LockImageBits<BitmapTraits>::iterator dst_end = dstBits.end();
#endif
      int src_r, src_g, src_b, src_a;
      int dst_r, dst_g, dst_b, dst_a;
      color_t c;

      dst_r = rgba_getr(color);
      dst_g = rgba_getg(color);
      dst_b = rgba_getb(color);
      dst_a = rgba_geta(color);

      for (; src_it != src_end; ++src_it, ++dst_it) {
        ASSERT(dst_it != dst_end);
        c = *src_it;

        src_r = rgba_getr(c);
        src_g = rgba_getg(c);
        src_b = rgba_getb(c);
        src_a = rgba_geta(c);

        if (!((src_r >= dst_r-fuzziness) && (src_r <= dst_r+fuzziness) &&
              (src_g >= dst_g-fuzziness) && (src_g <= dst_g+fuzziness) &&
              (src_b >= dst_b-fuzziness) && (src_b <= dst_b+fuzziness) &&
              (src_a >= dst_a-fuzziness) && (src_a <= dst_a+fuzziness)))
          *dst_it = 0;
      }
      ASSERT(dst_it == dst_end);
      break;
    }

    case IMAGE_GRAYSCALE: {
      const LockImageBits<GrayscaleTraits> srcBits(src);
      LockImageBits<BitmapTraits> dstBits(dst, Image::WriteLock);
      LockImageBits<GrayscaleTraits>::const_iterator src_it = srcBits.begin(), src_end = srcBits.end();
      LockImageBits<BitmapTraits>::iterator dst_it = dstBits.begin();
#ifdef _DEBUG
      LockImageBits<BitmapTraits>::iterator dst_end = dstBits.end();
#endif
      int src_k, src_a;
      int dst_k, dst_a;
      color_t c;

      dst_k = graya_getv(color);
      dst_a = graya_geta(color);

      for (; src_it != src_end; ++src_it, ++dst_it) {
        ASSERT(dst_it != dst_end);
        c = *src_it;

        src_k = graya_getv(c);
        src_a = graya_geta(c);

        if (!((src_k >= dst_k-fuzziness) && (src_k <= dst_k+fuzziness) &&
              (src_a >= dst_a-fuzziness) && (src_a <= dst_a+fuzziness)))
          *dst_it = 0;
      }
      ASSERT(dst_it == dst_end);
      break;
    }

    case IMAGE_INDEXED: {
      const LockImageBits<IndexedTraits> srcBits(src);
      LockImageBits<BitmapTraits> dstBits(dst, Image::WriteLock);
      LockImageBits<IndexedTraits>::const_iterator src_it = srcBits.begin(), src_end = srcBits.end();
      LockImageBits<BitmapTraits>::iterator dst_it = dstBits.begin();
#ifdef _DEBUG
      LockImageBits<BitmapTraits>::iterator dst_end = dstBits.end();
#endif
      color_t c, min, max;

      for (; src_it != src_end; ++src_it, ++dst_it) {
        ASSERT(dst_it != dst_end);
        c = *src_it;

        if (color > fuzziness)
          min = color-fuzziness;
        else
          min = 0;
        max = color + fuzziness;

        if (!((c >= min) && (c <= max)))
          *dst_it = 0;
      }
      ASSERT(dst_it == dst_end);
      break;
    }
  }

  shrink();
}

void Mask::crop(const Image *image)
{
#define ADVANCE(beg, end, o_end, cmp, op, getpixel1, getpixel)  \
  {                                                             \
    done = true;                                                \
    for (beg=beg_##beg; beg cmp beg_##end; beg op) {            \
      old_color = getpixel1;                                    \
      done = true;                                              \
      for (c++; c<=beg_##o_end; c++) {                          \
        if (getpixel != old_color) {                            \
          done = false;                                         \
          break;                                                \
        }                                                       \
      }                                                         \
      if (!done)                                                \
        break;                                                  \
    }                                                           \
    if (done)                                                   \
      done_count++;                                             \
  }

  int beg_x1, beg_y1, beg_x2, beg_y2;
  int c, x1, y1, x2, y2;
  int done_count = 0;
  int done;
  color_t old_color;

  if (!m_bitmap)
    return;

  beg_x1 = m_bounds.x;
  beg_y1 = m_bounds.y;
  beg_x2 = beg_x1 + m_bounds.w - 1;
  beg_y2 = beg_y1 + m_bounds.h - 1;

  beg_x1 = base::clamp(beg_x1, 0, m_bounds.w-1);
  beg_y1 = base::clamp(beg_y1, 0, m_bounds.h-1);
  beg_x2 = base::clamp(beg_x2, beg_x1, m_bounds.w-1);
  beg_y2 = base::clamp(beg_y2, beg_y1, m_bounds.h-1);

  /* left */
  ADVANCE(x1, x2, y2, <=, ++,
          get_pixel(image, x1, c=beg_y1),
          get_pixel(image, x1, c));
  /* right */
  ADVANCE(x2, x1, y2, >=, --,
          get_pixel(image, x2, c=beg_y1),
          get_pixel(image, x2, c));
  /* top */
  ADVANCE(y1, y2, x2, <=, ++,
          get_pixel(image, c=beg_x1, y1),
          get_pixel(image, c, y1));
  /* bottom */
  ADVANCE(y2, y1, x2, >=, --,
          get_pixel(image, c=beg_x1, y2),
          get_pixel(image, c, y2));

  if (done_count < 4)
    intersect(gfx::Rect(x1, y1, x2-x1+1, y2-y1+1));
  else
    clear();

#undef ADVANCE
}

void Mask::reserve(const gfx::Rect& bounds)
{
  ASSERT(!bounds.isEmpty());

  if (!m_bitmap) {
    m_bounds = bounds;
    m_bitmap.reset(Image::create(IMAGE_BITMAP, bounds.w, bounds.h, m_buffer));
    clear_image(m_bitmap.get(), 0);
  }
  else {
    gfx::Rect newBounds = m_bounds.createUnion(bounds);

    if (m_bounds != newBounds) {
      Image* image = crop_image(
        m_bitmap.get(),
        newBounds.x-m_bounds.x,
        newBounds.y-m_bounds.y,
        newBounds.w,
        newBounds.h, 0);
      m_bitmap.reset(image);
      m_bounds = newBounds;
    }
  }
}

void Mask::shrink()
{
  // If the mask is frozen we avoid the shrinking
  if (m_freeze_count > 0)
    return;

#define SHRINK_SIDE(u_begin, u_op, u_final, u_add,                      \
                    v_begin, v_op, v_final, v_add, U, V, var)           \
  {                                                                     \
    for (u = u_begin; u u_op u_final; u u_add) {                        \
      for (v = v_begin; v v_op v_final; v v_add) {                      \
        if (get_pixel_fast<BitmapTraits>(m_bitmap.get(), U, V))         \
          break;                                                        \
      }                                                                 \
      if (v == v_final)                                                 \
        var;                                                            \
      else                                                              \
        break;                                                          \
    }                                                                   \
  }

  int u, v, x1, y1, x2, y2;

  x1 = m_bounds.x;
  y1 = m_bounds.y;
  x2 = m_bounds.x+m_bounds.w-1;
  y2 = m_bounds.y+m_bounds.h-1;

  SHRINK_SIDE(0, <, m_bounds.w, ++,
              0, <, m_bounds.h, ++, u, v, x1++);

  SHRINK_SIDE(0, <, m_bounds.h, ++,
              0, <, m_bounds.w, ++, v, u, y1++);

  SHRINK_SIDE(m_bounds.w-1, >, 0, --,
              0, <, m_bounds.h, ++, u, v, x2--);

  SHRINK_SIDE(m_bounds.h-1, >, 0, --,
              0, <, m_bounds.w, ++, v, u, y2--);

  if ((x1 > x2) || (y1 > y2)) {
    clear();
  }
  else if ((x1 != m_bounds.x) || (x2 != m_bounds.x+m_bounds.w-1) ||
           (y1 != m_bounds.y) || (y2 != m_bounds.y+m_bounds.h-1)) {
    u = m_bounds.x;
    v = m_bounds.y;

    m_bounds.x = x1;
    m_bounds.y = y1;
    m_bounds.w = x2 - x1 + 1;
    m_bounds.h = y2 - y1 + 1;

    Image* image = crop_image(
      m_bitmap.get(),
      m_bounds.x-u, m_bounds.y-v,
      m_bounds.w, m_bounds.h, 0);
    m_bitmap.reset(image);
  }

#undef SHRINK_SIDE
}

} // namespace doc
