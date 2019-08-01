// Aseprite Document Library
// Copyright (C) 2018-2019  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_IMAGE_IMPL_H_INCLUDED
#define DOC_IMAGE_IMPL_H_INCLUDED
#pragma once

#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "doc/blend_funcs.h"
#include "doc/image.h"
#include "doc/image_bits.h"
#include "doc/image_iterator.h"
#include "doc/palette.h"

namespace doc {

  template<typename ImageTraits> class LockImageBits;

  template<class Traits>
  class ImageImpl : public Image {
  private:
    typedef typename Traits::address_t address_t;
    typedef typename Traits::const_address_t const_address_t;

    ImageBufferPtr m_buffer;
    address_t m_bits;
    address_t* m_rows;

    inline address_t getBitsAddress() {
      return m_bits;
    }

    inline const_address_t getBitsAddress() const {
      return m_bits;
    }

    inline address_t getLineAddress(int y) {
      ASSERT(y >= 0 && y < height());
      return m_rows[y];
    }

    inline const_address_t getLineAddress(int y) const {
      ASSERT(y >= 0 && y < height());
      return m_rows[y];
    }

  public:
    inline address_t address(int x, int y) const {
      return (address_t)(m_rows[y] + x / (Traits::pixels_per_byte == 0 ? 1 : Traits::pixels_per_byte));
    }

    ImageImpl(const ImageSpec& spec,
              const ImageBufferPtr& buffer)
      : Image(spec)
      , m_buffer(buffer)
    {
      ASSERT(Traits::color_mode == spec.colorMode());

      std::size_t for_rows = sizeof(address_t) * spec.height();
      std::size_t rowstride_bytes = Traits::getRowStrideBytes(spec.width());
      std::size_t required_size = for_rows + rowstride_bytes*spec.height();

      if (!m_buffer)
        m_buffer = std::make_shared<ImageBuffer>(required_size);
      else
        m_buffer->resizeIfNecessary(required_size);

      m_rows = (address_t*)m_buffer->buffer();
      m_bits = (address_t)(m_buffer->buffer() + for_rows);

      address_t addr = m_bits;
      for (int y=0; y<spec.height(); ++y) {
        m_rows[y] = addr;
        addr = (address_t)(((uint8_t*)addr) + rowstride_bytes);
      }
    }

    uint8_t* getPixelAddress(int x, int y) const override {
      ASSERT(x >= 0 && x < width());
      ASSERT(y >= 0 && y < height());

      return (uint8_t*)address(x, y);
    }

    color_t getPixel(int x, int y) const override {
      ASSERT(x >= 0 && x < width());
      ASSERT(y >= 0 && y < height());

      return *address(x, y);
    }

    void putPixel(int x, int y, color_t color) override {
      ASSERT(x >= 0 && x < width());
      ASSERT(y >= 0 && y < height());

      *address(x, y) = color;
    }

    void clear(color_t color) override {
      int w = width();
      int h = height();

      // Fill the first line
      address_t first = address(0, 0);
      std::fill(first, first+w, color);

      // Copy the first line into all other lines
      for (int y=1; y<h; ++y)
        std::copy(first, first+w, address(0, y));
    }

    void copy(const Image* _src, gfx::Clip area) override {
      const ImageImpl<Traits>* src = (const ImageImpl<Traits>*)_src;
      address_t src_address;
      address_t dst_address;

      if (!area.clip(width(), height(), src->width(), src->height()))
        return;

      for (int end_y=area.dst.y+area.size.h;
           area.dst.y<end_y;
           ++area.dst.y, ++area.src.y) {
        src_address = src->address(area.src.x, area.src.y);
        dst_address = address(area.dst.x, area.dst.y);

        std::copy(src_address,
                  src_address + area.size.w,
                  dst_address);
      }
    }

    void drawHLine(int x1, int y, int x2, color_t color) override {
      LockImageBits<Traits> bits(this, gfx::Rect(x1, y, x2 - x1 + 1, 1));
      typename LockImageBits<Traits>::iterator it(bits.begin());
      typename LockImageBits<Traits>::iterator end(bits.end());

      for (; it != end; ++it)
        *it = color;
    }

    void fillRect(int x1, int y1, int x2, int y2, color_t color) override {
      // Fill the first line
      ImageImpl<Traits>::drawHLine(x1, y1, x2, color);

      // Copy all other lines
      address_t first = address(x1, y1);
      int w = x2 - x1 + 1;
      for (int y=y1; y<=y2; ++y)
        std::copy(first, first+w, address(x1, y));
    }

    void blendRect(int x1, int y1, int x2, int y2, color_t color, int opacity) override {
      fillRect(x1, y1, x2, y2, color);
    }

  private:
    bool clip_rects(const Image* src, int& dst_x, int& dst_y, int& src_x, int& src_y, int& w, int& h) const {
      // Clip with destionation image
      if (dst_x < 0) {
        w += dst_x;
        src_x -= dst_x;
        dst_x = 0;
      }
      if (dst_y < 0) {
        h += dst_y;
        src_y -= dst_y;
        dst_y = 0;
      }
      if (dst_x+w > width()) {
        w = width() - dst_x;
      }
      if (dst_y+h > height()) {
        h = height() - dst_y;
      }

      // Clip with source image
      if (src_x < 0) {
        w += src_x;
        dst_x -= src_x;
        src_x = 0;
      }
      if (src_y < 0) {
        h += src_y;
        dst_y -= src_y;
        src_y = 0;
      }
      if (src_x+w > src->width()) {
        w = src->width() - src_x;
      }
      if (src_y+h > src->height()) {
        h = src->height() - src_y;
      }

      // Empty cases
      if (w < 1 || h < 1)
        return false;

      if ((src_x+w <= 0) || (src_x >= src->width()) ||
          (src_y+h <= 0) || (src_y >= src->height()))
        return false;

      if ((dst_x+w <= 0) || (dst_x >= width()) ||
          (dst_y+h <= 0) || (dst_y >= height()))
        return false;

      // Check this function is working correctly
      ASSERT(src->bounds().contains(gfx::Rect(src_x, src_y, w, h)));
      ASSERT(bounds().contains(gfx::Rect(dst_x, dst_y, w, h)));
      return true;
    }
  };

  //////////////////////////////////////////////////////////////////////
  // Specializations

  template<>
  inline void ImageImpl<IndexedTraits>::clear(color_t color) {
    std::fill(m_bits,
              m_bits + width()*height(),
              color);
  }

  template<>
  inline void ImageImpl<BitmapTraits>::clear(color_t color) {
    std::fill(m_bits,
              m_bits + BitmapTraits::getRowStrideBytes(width()) * height(),
              (color ? 0xff: 0x00));
  }

  template<>
  inline color_t ImageImpl<BitmapTraits>::getPixel(int x, int y) const {
    ASSERT(x >= 0 && x < width());
    ASSERT(y >= 0 && y < height());

    std::div_t d = std::div(x, 8);
    return ((*(m_rows[y] + d.quot)) & (1<<d.rem)) ? 1: 0;
  }

  template<>
  inline void ImageImpl<BitmapTraits>::putPixel(int x, int y, color_t color) {
    ASSERT(x >= 0 && x < width());
    ASSERT(y >= 0 && y < height());

    std::div_t d = std::div(x, 8);
    if (color)
      (*(m_rows[y] + d.quot)) |= (1 << d.rem);
    else
      (*(m_rows[y] + d.quot)) &= ~(1 << d.rem);
  }

  template<>
  inline void ImageImpl<BitmapTraits>::fillRect(int x1, int y1, int x2, int y2, color_t color) {
    for (int y=y1; y<=y2; ++y)
      ImageImpl<BitmapTraits>::drawHLine(x1, y, x2, color);
  }

  template<>
  inline void ImageImpl<RgbTraits>::blendRect(int x1, int y1, int x2, int y2, color_t color, int opacity) {
    address_t addr;
    int x, y;

    for (y=y1; y<=y2; ++y) {
      addr = (address_t)getPixelAddress(x1, y);
      for (x=x1; x<=x2; ++x) {
        *addr = rgba_blender_normal(*addr, color, opacity);
        ++addr;
      }
    }
  }

  void copy_bitmaps(Image* dst, const Image* src, gfx::Clip area);
  template<>
  inline void ImageImpl<BitmapTraits>::copy(const Image* src, gfx::Clip area) {
    copy_bitmaps(this, src, area);
  }

} // namespace doc

#endif
