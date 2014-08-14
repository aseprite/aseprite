/* Aseprite
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "raster/mask.h"

#include "base/memory.h"
#include "raster/image.h"
#include "raster/image_bits.h"

#include <cstdlib>
#include <cstring>

namespace raster {

Mask::Mask()
  : Object(OBJECT_MASK)
{
  initialize();
}

Mask::Mask(const Mask& mask)
  : Object(mask)
{
  initialize();
  copyFrom(&mask);
}

Mask::Mask(int x, int y, Image* bitmap)
  : Object(OBJECT_MASK)
  , m_freeze_count(0)
  , m_bounds(x, y, bitmap->width(), bitmap->height())
  , m_bitmap(bitmap)
{
}

Mask::~Mask()
{
  ASSERT(m_freeze_count == 0);
  delete m_bitmap;
}

void Mask::initialize()
{
  m_freeze_count = 0;
  m_bounds = gfx::Rect(0, 0, 0, 0);
  m_bitmap = NULL;
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

  LockImageBits<BitmapTraits> bits(m_bitmap);
  LockImageBits<BitmapTraits>::iterator it = bits.begin(), end = bits.end();

  for (; it != end; ++it) {
    if (*it == 0)
      return false;
  }

  return true;
}

void Mask::copyFrom(const Mask* sourceMask)
{
  clear();
  setName(sourceMask->name().c_str());

  if (sourceMask->bitmap()) {
    // Add all the area of "mask"
    add(sourceMask->bounds());

    // And copy the "mask" bitmap
    copy_image(m_bitmap, sourceMask->m_bitmap, 0, 0);
  }
}

void Mask::offsetOrigin(int dx, int dy)
{
  m_bounds.offset(dx, dy);
}

void Mask::clear()
{
  if (m_bitmap) {
    delete m_bitmap;
    m_bitmap = NULL;
  }
  m_bounds = gfx::Rect(0, 0, 0, 0);
}

void Mask::invert()
{
  if (m_bitmap) {
    LockImageBits<BitmapTraits> bits(m_bitmap);
    LockImageBits<BitmapTraits>::iterator it = bits.begin(), end = bits.end();

    for (; it != end; ++it)
      *it = (*it ? 0: 1);

    shrink();
  }
}

void Mask::replace(int x, int y, int w, int h)
{
  m_bounds = gfx::Rect(x, y, w, h);

  delete m_bitmap;
  m_bitmap = Image::create(IMAGE_BITMAP, w, h);

  clear_image(m_bitmap, 1);
}

void Mask::replace(const gfx::Rect& bounds)
{
  replace(bounds.x, bounds.y, bounds.w, bounds.h);
}

void Mask::add(int x, int y, int w, int h)
{
  if (m_freeze_count == 0)
    reserve(x, y, w, h);

  fill_rect(m_bitmap,
            x-m_bounds.x, y-m_bounds.y,
            x-m_bounds.x+w-1, y-m_bounds.y+h-1, 1);
}

void Mask::add(const gfx::Rect& bounds)
{
  add(bounds.x, bounds.y, bounds.w, bounds.h);
}

void Mask::subtract(int x, int y, int w, int h)
{
  if (m_bitmap) {
    fill_rect(m_bitmap,
              x-m_bounds.x,
              y-m_bounds.y,
              x-m_bounds.x+w-1,
              y-m_bounds.y+h-1, 0);
    shrink();
  }
}

void Mask::subtract(const gfx::Rect& bounds)
{
  subtract(bounds.x, bounds.y, bounds.w, bounds.h);
}

void Mask::intersect(int x, int y, int w, int h)
{
  if (m_bitmap) {
    int x1 = m_bounds.x;
    int y1 = m_bounds.y;
    int x2 = MIN(m_bounds.x+m_bounds.w-1, x+w-1);
    int y2 = MIN(m_bounds.y+m_bounds.h-1, y+h-1);

    m_bounds.x = MAX(x, x1);
    m_bounds.y = MAX(y, y1);
    m_bounds.w = x2 - m_bounds.x + 1;
    m_bounds.h = y2 - m_bounds.y + 1;

    Image* image = crop_image(m_bitmap, m_bounds.x-x1, m_bounds.y-y1, m_bounds.w, m_bounds.h, 0);
    delete m_bitmap;
    m_bitmap = image;

    shrink();
  }
}

void Mask::intersect(const gfx::Rect& bounds)
{
  subtract(bounds.x, bounds.y, bounds.w, bounds.h);
}

void Mask::byColor(const Image *src, int color, int fuzziness)
{
  replace(0, 0, src->width(), src->height());

  Image* dst = m_bitmap;

  switch (src->pixelFormat()) {

    case IMAGE_RGB: {
      const LockImageBits<RgbTraits> srcBits(src);
      LockImageBits<BitmapTraits> dstBits(dst, Image::WriteLock);
      LockImageBits<RgbTraits>::const_iterator src_it = srcBits.begin(), src_end = srcBits.end();
      LockImageBits<BitmapTraits>::iterator dst_it = dstBits.begin(), dst_end = dstBits.end();
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
      LockImageBits<BitmapTraits>::iterator dst_it = dstBits.begin(), dst_end = dstBits.end();
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
      LockImageBits<BitmapTraits>::iterator dst_it = dstBits.begin(), dst_end = dstBits.end();
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
  int c, x1, y1, x2, y2, old_color;
  int done_count = 0;
  int done;

  if (!m_bitmap)
    return;

  beg_x1 = m_bounds.x;
  beg_y1 = m_bounds.y;
  beg_x2 = beg_x1 + m_bounds.w - 1;
  beg_y2 = beg_y1 + m_bounds.h - 1;

  beg_x1 = MID(0, beg_x1, m_bounds.w-1);
  beg_y1 = MID(0, beg_y1, m_bounds.h-1);
  beg_x2 = MID(beg_x1, beg_x2, m_bounds.w-1);
  beg_y2 = MID(beg_y1, beg_y2, m_bounds.h-1);

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
    intersect(x1, y1, x2, y2);
  else
    clear();

#undef ADVANCE
}

void Mask::reserve(int x, int y, int w, int h)
{
  ASSERT(w > 0 && h > 0);

  if (!m_bitmap) {
    m_bounds.x = x;
    m_bounds.y = y;
    m_bounds.w = w;
    m_bounds.h = h;
    m_bitmap = Image::create(IMAGE_BITMAP, w, h);
    clear_image(m_bitmap, 0);
  }
  else {
    int x1 = m_bounds.x;
    int y1 = m_bounds.y;
    int x2 = MAX(m_bounds.x+m_bounds.w-1, x+w-1);
    int y2 = MAX(m_bounds.y+m_bounds.h-1, y+h-1);
    int new_mask_x = MIN(x, x1);
    int new_mask_y = MIN(y, y1);
    int new_mask_w = x2 - new_mask_x + 1;
    int new_mask_h = y2 - new_mask_y + 1;

    if (m_bounds.x != new_mask_x ||
        m_bounds.y != new_mask_y ||
        m_bounds.w != new_mask_w ||
        m_bounds.h != new_mask_h) {
      m_bounds.x = new_mask_x;
      m_bounds.y = new_mask_y;
      m_bounds.w = new_mask_w;
      m_bounds.h = new_mask_h;

      Image* image = crop_image(m_bitmap, m_bounds.x-x1, m_bounds.y-y1, m_bounds.w, m_bounds.h, 0);
      delete m_bitmap;      // image
      m_bitmap = image;
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
        if (m_bitmap->getPixel(U, V))                                   \
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

    Image* image = crop_image(m_bitmap, m_bounds.x-u, m_bounds.y-v, m_bounds.w, m_bounds.h, 0);
    delete m_bitmap;
    m_bitmap = image;
  }

#undef SHRINK_SIDE
}

} // namespace raster
