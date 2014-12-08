// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_IMAGE_IMPL_H_INCLUDED
#define DOC_IMAGE_IMPL_H_INCLUDED
#pragma once

#include "doc/blend.h"
#include "doc/image.h"
#include "doc/image_bits.h"
#include "doc/image_iterator.h"
#include "doc/palette.h"

namespace doc {

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

    ImageImpl(int width, int height,
              const ImageBufferPtr& buffer)
      : Image(static_cast<PixelFormat>(Traits::pixel_format), width, height)
      , m_buffer(buffer)
    {
      size_t for_rows = sizeof(address_t) * height;
      size_t rowstride_bytes = Traits::getRowStrideBytes(width);
      size_t required_size = for_rows + rowstride_bytes*height;

      if (!m_buffer)
        m_buffer.reset(new ImageBuffer(required_size));
      else
        m_buffer->resizeIfNecessary(required_size);

      m_rows = (address_t*)m_buffer->buffer();
      m_bits = (address_t)(m_buffer->buffer() + for_rows);

      address_t addr = m_bits;
      for (int y=0; y<height; ++y) {
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
      LockImageBits<Traits> bits(this);
      typename LockImageBits<Traits>::iterator it(bits.begin());
      typename LockImageBits<Traits>::iterator end(bits.end());

      for (; it != end; ++it)
        *it = color;
    }

    void copy(const Image* _src, int dst_x, int dst_y, int src_x, int src_y, int w, int h) override {
      const ImageImpl<Traits>* src = (const ImageImpl<Traits>*)_src;
      address_t src_address;
      address_t dst_address;
      int bytes;

      if (!clip_rects(src, dst_x, dst_y, src_x, src_y, w, h))
        return;

      // Copy process
      bytes = Traits::getRowStrideBytes(w);

      for (int end_y=dst_y+h; dst_y<end_y; ++dst_y, ++src_y) {
        src_address = src->address(src_x, src_y);
        dst_address = address(dst_x, dst_y);

        memcpy(dst_address, src_address, bytes);
      }
    }

    void merge(const Image* _src, int dst_x, int dst_y, int src_x, int src_y, int w, int h, int opacity, int blend_mode) override {
      BLEND_COLOR blender = Traits::get_blender(blend_mode);
      const ImageImpl<Traits>* src = (const ImageImpl<Traits>*)_src;
      ImageImpl<Traits>* dst = this;
      address_t src_address;
      address_t dst_address;
      uint32_t mask_color = src->maskColor();

      // nothing to do
      if (!opacity)
        return;

      if (!clip_rects(src, dst_x, dst_y, src_x, src_y, w, h))
        return;

      // Merge process
      int end_x = dst_x+w;
      for (int end_y=dst_y+h; dst_y<end_y; ++dst_y, ++src_y) {
        src_address = src->address(src_x, src_y);
        dst_address = dst->address(dst_x, dst_y);

        for (int x=dst_x; x<end_x; ++x) {
          if (*src_address != mask_color)
            *dst_address = (*blender)(*dst_address, *src_address, opacity);

          ++dst_address;
          ++src_address;
        }
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
      for (int y=y1; y<=y2; ++y)
        ImageImpl<Traits>::drawHLine(x1, y, x2, color);
    }

    void blendRect(int x1, int y1, int x2, int y2, color_t color, int opacity) override {
      fillRect(x1, y1, x2, y2, color);
    }

  private:
    bool clip_rects(const Image* src, int& dst_x, int& dst_y, int& src_x, int& src_y, int& w, int& h) const {
      // Clip with destionation image
      if (dst_x < 0) {
        src_x -= dst_x;
        dst_x = 0;
      }
      if (dst_y < 0) {
        src_y -= dst_y;
        dst_y = 0;
      }
      if (dst_x+w > width()) {
        w = width() - dst_x;
        if (w < 0)
          return false;
      }
      if (dst_y+h > height()) {
        h = height() - dst_y;
        if (h < 0)
          return false;
      }

      // Clip with source image
      if (src_x < 0) {
        dst_x -= src_x;
        src_x = 0;
      }
      if (src_y < 0) {
        dst_y -= src_y;
        src_y = 0;
      }
      if (src_x+w > src->width()) {
        w = src->width() - src_x;
        if (w < 0)
          return false;
      }
      if (src_y+h > src->height()) {
        h = src->height() - src_y;
        if (h < 0)
          return false;
      }

      // Empty cases
      if ((src_x+w <= 0) || (src_x >= src->width()) ||
          (src_y+h <= 0) || (src_y >= src->height()))
        return false;

      if ((dst_x+w <= 0) || (dst_x >= width()) ||
          (dst_y+h <= 0) || (dst_y >= height()))
        return false;

      ASSERT(src->bounds().contains(gfx::Rect(src_x, src_y, w, h)));
      ASSERT(bounds().contains(gfx::Rect(dst_x, dst_y, w, h)));

      return true;
    }
  };

  //////////////////////////////////////////////////////////////////////
  // Specializations

  template<>
  inline void ImageImpl<IndexedTraits>::clear(color_t color) {
    memset(m_bits, color, width()*height());
  }

  template<>
  inline void ImageImpl<BitmapTraits>::clear(color_t color) {
    memset(m_bits, (color ? 0xff: 0x00),
           BitmapTraits::getRowStrideBytes(width()) * height());
  }

  template<>
  inline color_t ImageImpl<BitmapTraits>::getPixel(int x, int y) const {
    ASSERT(x >= 0 && x < width());
    ASSERT(y >= 0 && y < height());

    div_t d = div(x, 8);
    return ((*(m_rows[y] + d.quot)) & (1<<d.rem)) ? 1: 0;
  }

  template<>
  inline void ImageImpl<BitmapTraits>::putPixel(int x, int y, color_t color) {
    ASSERT(x >= 0 && x < width());
    ASSERT(y >= 0 && y < height());

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
  inline void ImageImpl<IndexedTraits>::merge(const Image* src, int dst_x, int dst_y, int src_x, int src_y, int w, int h, int opacity, int blend_mode) {
    if (!clip_rects(src, dst_x, dst_y, src_x, src_y, w, h))
      return;

    address_t src_address;
    address_t dst_address;

    int end_x = dst_x+w;

    // Direct copy
    if (blend_mode == BLEND_MODE_COPY) {
      for (int end_y=dst_y+h; dst_y<end_y; ++dst_y, ++src_y) {
        src_address = src->getPixelAddress(src_x, src_y);
        dst_address = getPixelAddress(dst_x, dst_y);

        for (int x=dst_x; x<end_x; ++x) {
          *dst_address = (*src_address);

          ++dst_address;
          ++src_address;
        }
      }
    }
    // With mask
    else {
      int mask_color = src->maskColor();

      for (int end_y=dst_y+h; dst_y<end_y; ++dst_y, ++src_y) {
        src_address = src->getPixelAddress(src_x, src_y);
        dst_address = getPixelAddress(dst_x, dst_y);

        for (int x=dst_x; x<end_x; ++x) {
          if (*src_address != mask_color)
            *dst_address = (*src_address);

          ++dst_address;
          ++src_address;
        }
      }
    }
  }

  template<>
  inline void ImageImpl<BitmapTraits>::copy(const Image* src, int dst_x, int dst_y, int src_x, int src_y, int w, int h) {
    if (!clip_rects(src, dst_x, dst_y, src_x, src_y, w, h))
      return;

    // Copy process
    ImageConstIterator<BitmapTraits> src_it(src, gfx::Rect(src_x, src_y, w, h), src_x, src_y);
    ImageIterator<BitmapTraits> dst_it(this, gfx::Rect(dst_x, dst_y, w, h), dst_x, dst_y);

    int end_x = dst_x+w;

    for (int end_y=dst_y+h; dst_y<end_y; ++dst_y, ++src_y) {
      for (int x=dst_x; x<end_x; ++x) {
        *dst_it = *src_it;
        ++src_it;
        ++dst_it;
      }
    }
  }

  template<>
  inline void ImageImpl<BitmapTraits>::merge(const Image* src, int dst_x, int dst_y, int src_x, int src_y, int w, int h, int opacity, int blend_mode) {
    if (!clip_rects(src, dst_x, dst_y, src_x, src_y, w, h))
      return;

    // Merge process
    ImageConstIterator<BitmapTraits> src_it(src, gfx::Rect(src_x, src_y, w, h), src_x, src_y);
    ImageIterator<BitmapTraits> dst_it(this, gfx::Rect(dst_x, dst_y, w, h), dst_x, dst_y);

    int end_x = dst_x+w;

    for (int end_y=dst_y+h; dst_y<end_y; ++dst_y, ++src_y) {
      for (int x=dst_x; x<end_x; ++x) {
        if (*dst_it != 0)
          *dst_it = *src_it;
        ++src_it;
        ++dst_it;
      }
    }
  }

} // namespace doc

#endif
