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

#ifndef RASTER_IMAGE_IMPL_H_INCLUDED
#define RASTER_IMAGE_IMPL_H_INCLUDED

#include "raster/blend.h"
#include "raster/image.h"
#include "raster/image_bits.h"
#include "raster/image_iterator.h"
#include "raster/palette.h"

namespace raster {

  template<class Traits>
  class ImageImpl : public Image {
  private:
    typedef typename Traits::address_t address_t;
    typedef typename Traits::const_address_t const_address_t;

    address_t m_bits;            // Pixmap data.
    address_t* m_rows;           // Start of each scanline.

    inline address_t getBitsAddress() {
      return m_bits;
    }

    inline const_address_t getBitsAddress() const {
      return m_bits;
    }

    inline address_t getLineAddress(int y) {
      ASSERT(y >= 0 && y < getHeight());
      return m_rows[y];
    }

    inline const_address_t getLineAddress(int y) const {
      ASSERT(y >= 0 && y < getHeight());
      return m_rows[y];
    }

  public:
    ImageImpl(int width, int height)
      : Image(static_cast<PixelFormat>(Traits::pixel_format), width, height)
    {
      int rowstrideBytes = Traits::getRowStrideBytes(width);

      m_bits = (address_t)new uint8_t[rowstrideBytes * height];
      try {
        m_rows = new address_t[height];
      }
      catch (...) {
        delete[] m_bits;
        throw;
      }

      address_t addr = m_bits;
      for (int y=0; y<getHeight(); ++y) {
        m_rows[y] = addr;
        addr = (address_t)(((uint8_t*)addr) + rowstrideBytes);
      }
    }

    ~ImageImpl() {
      if (m_bits) delete[] m_bits;
      if (m_rows) delete[] m_rows;
    }

    uint8_t* getPixelAddress(int x, int y) const OVERRIDE {
      ASSERT(x >= 0 && x < getWidth());
      ASSERT(y >= 0 && y < getHeight());

      return (uint8_t*)(m_rows[y] + x);
    }

    color_t getPixel(int x, int y) const OVERRIDE {
      ASSERT(x >= 0 && x < getWidth());
      ASSERT(y >= 0 && y < getHeight());

      return (*(address_t)getPixelAddress(x, y));
    }

    void putPixel(int x, int y, color_t color) OVERRIDE {
      ASSERT(x >= 0 && x < getWidth());
      ASSERT(y >= 0 && y < getHeight());

      *(address_t)getPixelAddress(x, y) = color;
    }

    void clear(color_t color) OVERRIDE {
      LockImageBits<Traits> bits(this);
      LockImageBits<Traits>::iterator it(bits.begin());
      LockImageBits<Traits>::iterator end(bits.end());

      for (; it != end; ++it)
        *it = color;
    }

    void copy(const Image* src, int x, int y) OVERRIDE {
      Image* dst = this;
      address_t src_address;
      address_t dst_address;
      int xbeg, xend, xsrc;
      int ybeg, yend, ysrc, ydst;
      int bytes;

      // Clipping

      xsrc = 0;
      ysrc = 0;

      xbeg = x;
      ybeg = y;
      xend = x+src->getWidth()-1;
      yend = y+src->getHeight()-1;

      if ((xend < 0) || (xbeg >= dst->getWidth()) ||
          (yend < 0) || (ybeg >= dst->getHeight()))
        return;

      if (xbeg < 0) {
        xsrc -= xbeg;
        xbeg = 0;
      }

      if (ybeg < 0) {
        ysrc -= ybeg;
        ybeg = 0;
      }

      if (xend >= dst->getWidth())
        xend = dst->getWidth()-1;

      if (yend >= dst->getHeight())
        yend = dst->getHeight()-1;

      // Copy process

      bytes = Traits::getRowStrideBytes(xend - xbeg + 1);

      for (ydst=ybeg; ydst<=yend; ++ydst, ++ysrc) {
        src_address = (address_t)src->getPixelAddress(xsrc, ysrc);
        dst_address = (address_t)dst->getPixelAddress(xbeg, ydst);

        memcpy(dst_address, src_address, bytes);
      }
    }

    void merge(const Image* src, int x, int y, int opacity, int blend_mode) OVERRIDE {
      BLEND_COLOR blender = Traits::get_blender(blend_mode);
      register uint32_t mask_color = src->getMaskColor();
      Image* dst = this;
      address_t src_address;
      address_t dst_address;
      int xbeg, xend, xsrc, xdst;
      int ybeg, yend, ysrc, ydst;

      // nothing to do
      if (!opacity)
        return;

      // clipping

      xsrc = 0;
      ysrc = 0;

      xbeg = x;
      ybeg = y;
      xend = x+src->getWidth()-1;
      yend = y+src->getHeight()-1;

      if ((xend < 0) || (xbeg >= dst->getWidth()) ||
          (yend < 0) || (ybeg >= dst->getHeight()))
        return;

      if (xbeg < 0) {
        xsrc -= xbeg;
        xbeg = 0;
      }

      if (ybeg < 0) {
        ysrc -= ybeg;
        ybeg = 0;
      }

      if (xend >= dst->getWidth())
        xend = dst->getWidth()-1;

      if (yend >= dst->getHeight())
        yend = dst->getHeight()-1;

      // Merge process

      for (ydst=ybeg; ydst<=yend; ++ydst, ++ysrc) {
        src_address = (address_t)src->getPixelAddress(xsrc, ysrc);
        dst_address = (address_t)dst->getPixelAddress(xbeg, ydst);

        for (xdst=xbeg; xdst<=xend; ++xdst) {
          if (*src_address != mask_color)
            *dst_address = (*blender)(*dst_address, *src_address, opacity);

          ++dst_address;
          ++src_address;
        }
      }
    }

    void drawHLine(int x1, int y, int x2, color_t color) OVERRIDE {
      LockImageBits<Traits> bits(this, gfx::Rect(x1, y, x2 - x1 + 1, 1));
      LockImageBits<Traits>::iterator it(bits.begin());
      LockImageBits<Traits>::iterator end(bits.end());

      for (; it != end; ++it)
        *it = color;
    }

    void fillRect(int x1, int y1, int x2, int y2, color_t color) OVERRIDE {
      for (int y=y1; y<=y2; ++y)
        ImageImpl<Traits>::drawHLine(x1, y, x2, color);
    }

    void blendRect(int x1, int y1, int x2, int y2, color_t color, int opacity) OVERRIDE {
      fillRect(x1, y1, x2, y2, color);
    }
  };

  //////////////////////////////////////////////////////////////////////
  // Specializations

  template<>
  inline uint8_t* ImageImpl<BitmapTraits>::getPixelAddress(int x, int y) const {
    return (uint8_t*)(m_rows[y] + x/8);
  }

  template<>
  inline color_t ImageImpl<BitmapTraits>::getPixel(int x, int y) const {
    ASSERT(x >= 0 && x < getWidth());
    ASSERT(y >= 0 && y < getHeight());

    div_t d = div(x, 8);
    return ((*(m_rows[y] + d.quot)) & (1<<d.rem)) ? 1: 0;
  }

  template<>
  inline void ImageImpl<BitmapTraits>::putPixel(int x, int y, color_t color) {
    ASSERT(x >= 0 && x < getWidth());
    ASSERT(y >= 0 && y < getHeight());

    div_t d = div(x, 8);
    if (color)
      (*(m_rows[y] + d.quot)) |= (1 << d.rem);
    else
      (*(m_rows[y] + d.quot)) &= ~(1 << d.rem);
  }

  template<>
  inline void ImageImpl<RgbTraits>::blendRect(int x1, int y1, int x2, int y2, color_t color, int opacity) {
    address_t addr;
    int x, y;

    for (y=y1; y<=y2; ++y) {
      addr = (address_t)getPixelAddress(x1, y);
      for (x=x1; x<=x2; ++x) {
        *addr = rgba_blend_normal(*addr, color, opacity);
        ++addr;
      }
    }
  }

  template<>
  inline void ImageImpl<IndexedTraits>::merge(const Image* src, int x, int y, int opacity, int blend_mode) {
    Image* dst = this;
    address_t src_address;
    address_t dst_address;
    int xbeg, xend, xsrc, xdst;
    int ybeg, yend, ysrc, ydst;

    // clipping

    xsrc = 0;
    ysrc = 0;

    xbeg = x;
    ybeg = y;
    xend = x+src->getWidth()-1;
    yend = y+src->getHeight()-1;

    if ((xend < 0) || (xbeg >= dst->getWidth()) ||
        (yend < 0) || (ybeg >= dst->getHeight()))
      return;

    if (xbeg < 0) {
      xsrc -= xbeg;
      xbeg = 0;
    }

    if (ybeg < 0) {
      ysrc -= ybeg;
      ybeg = 0;
    }

    if (xend >= dst->getWidth())
      xend = dst->getWidth()-1;

    if (yend >= dst->getHeight())
      yend = dst->getHeight()-1;

    // merge process

    // direct copy
    if (blend_mode == BLEND_MODE_COPY) {
      for (ydst=ybeg; ydst<=yend; ++ydst, ++ysrc) {
        src_address = src->getPixelAddress(xsrc, ysrc);
        dst_address = dst->getPixelAddress(xbeg, ydst);

        for (xdst=xbeg; xdst<=xend; xdst++) {
          *dst_address = (*src_address);

          ++dst_address;
          ++src_address;
        }
      }
    }
    // with mask
    else {
      register int mask_color = src->getMaskColor();

      for (ydst=ybeg; ydst<=yend; ++ydst, ++ysrc) {
        src_address = src->getPixelAddress(xsrc, ysrc);
        dst_address = dst->getPixelAddress(xbeg, ydst);

        for (xdst=xbeg; xdst<=xend; ++xdst) {
          if (*src_address != mask_color)
            *dst_address = (*src_address);

          ++dst_address;
          ++src_address;
        }
      }
    }
  }

  template<>
  inline void ImageImpl<BitmapTraits>::copy(const Image* src, int x, int y) {
    Image* dst = this;
    int xbeg, xend, xsrc, xdst;
    int ybeg, yend, ysrc, ydst;

    // clipping

    xsrc = 0;
    ysrc = 0;

    xbeg = x;
    ybeg = y;
    xend = x+src->getWidth()-1;
    yend = y+src->getHeight()-1;

    if ((xend < 0) || (xbeg >= dst->getWidth()) ||
        (yend < 0) || (ybeg >= dst->getHeight()))
      return;

    if (xbeg < 0) {
      xsrc -= xbeg;
      xbeg = 0;
    }

    if (ybeg < 0) {
      ysrc -= ybeg;
      ybeg = 0;
    }

    if (xend >= dst->getWidth())
      xend = dst->getWidth()-1;

    if (yend >= dst->getHeight())
      yend = dst->getHeight()-1;

    // copy process

    int w = xend - xbeg + 1;
    int h = yend - ybeg + 1;
    ImageConstIterator<BitmapTraits> src_it(src, gfx::Rect(xsrc, ysrc, w, h), xsrc, ysrc);
    ImageIterator<BitmapTraits> dst_it(dst, gfx::Rect(xbeg, ybeg, w, h), xbeg, ybeg);

    for (ydst=ybeg; ydst<=yend; ++ydst, ++ysrc) {
      for (xdst=xbeg; xdst<=xend; ++xdst) {
        *dst_it = *src_it;
        ++src_it;
        ++dst_it;
      }
    }
  }

  template<>
  inline void ImageImpl<BitmapTraits>::merge(const Image* src, int x, int y, int opacity, int blend_mode) {
    Image* dst = this;
    int xbeg, xend, xsrc, xdst;
    int ybeg, yend, ysrc, ydst;

    // clipping

    xsrc = 0;
    ysrc = 0;

    xbeg = x;
    ybeg = y;
    xend = x+src->getWidth()-1;
    yend = y+src->getHeight()-1;

    if ((xend < 0) || (xbeg >= dst->getWidth()) ||
        (yend < 0) || (ybeg >= dst->getHeight()))
      return;

    if (xbeg < 0) {
      xsrc -= xbeg;
      xbeg = 0;
    }

    if (ybeg < 0) {
      ysrc -= ybeg;
      ybeg = 0;
    }

    if (xend >= dst->getWidth())
      xend = dst->getWidth()-1;

    if (yend >= dst->getHeight())
      yend = dst->getHeight()-1;

    // merge process

    int w = xend - xbeg + 1;
    int h = yend - ybeg + 1;
    ImageConstIterator<BitmapTraits> src_it(src, gfx::Rect(xsrc, ysrc, w, h), xsrc, ysrc);
    ImageIterator<BitmapTraits> dst_it(dst, gfx::Rect(xbeg, ybeg, w, h), xbeg, ybeg);

    for (ydst=ybeg; ydst<=yend; ++ydst, ++ysrc) {
      for (xdst=xbeg; xdst<=xend; ++xdst) {
        if (*dst_it != 0)
          *dst_it = *src_it;
        ++src_it;
        ++dst_it;
      }
    }
  }

} // namespace raster

#endif
