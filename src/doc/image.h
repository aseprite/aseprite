// Aseprite Document Library
// Copyright (c) 2018 Igara Studio S.A.
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_IMAGE_H_INCLUDED
#define DOC_IMAGE_H_INCLUDED
#pragma once

#include "doc/color.h"
#include "doc/color_mode.h"
#include "doc/image_buffer.h"
#include "doc/image_spec.h"
#include "doc/object.h"
#include "doc/pixel_format.h"
#include "gfx/clip.h"
#include "gfx/rect.h"
#include "gfx/size.h"

namespace doc {

  template<typename ImageTraits> class ImageBits;
  class Palette;
  class Pen;
  class RgbMap;

  class Image : public Object {
  public:
    enum LockType {
      ReadLock,                 // Read-only lock
      WriteLock,                // Write-only lock
      ReadWriteLock             // Read and write
    };

    static Image* create(PixelFormat format, int width, int height,
                         const ImageBufferPtr& buffer = ImageBufferPtr());
    static Image* create(const ImageSpec& spec,
                         const ImageBufferPtr& buffer = ImageBufferPtr());
    static Image* createCopy(const Image* image,
                             const ImageBufferPtr& buffer = ImageBufferPtr());

    virtual ~Image();

    const ImageSpec& spec() const { return m_spec; }
    ColorMode colorMode() const { return m_spec.colorMode(); }
    PixelFormat pixelFormat() const { return (PixelFormat)colorMode(); }
    int width() const { return m_spec.width(); }
    int height() const { return m_spec.height(); }
    gfx::Size size() const { return m_spec.size(); }
    gfx::Rect bounds() const { return m_spec.bounds(); }
    color_t maskColor() const { return m_spec.maskColor(); }
    void setMaskColor(color_t c) { m_spec.setMaskColor(c); }
    void setColorSpace(const gfx::ColorSpacePtr& cs) { m_spec.setColorSpace(cs); }

    virtual int getMemSize() const override;
    int getRowStrideSize() const;
    int getRowStrideSize(int pixels_per_row) const;

    template<typename ImageTraits>
    ImageBits<ImageTraits> lockBits(LockType lockType, const gfx::Rect& bounds) {
      return ImageBits<ImageTraits>(this, bounds);
    }

    template<typename ImageTraits>
    ImageBits<ImageTraits> lockBits(LockType lockType, const gfx::Rect& bounds) const {
      return ImageBits<ImageTraits>(const_cast<Image*>(this), bounds);
    }

    template<typename ImageTraits>
    void unlockBits(ImageBits<ImageTraits>& imageBits) {
      // Do nothing
    }

    // Warning: These functions doesn't have (and shouldn't have)
    // bounds checks. Use the primitives defined in doc/primitives.h
    // in case that you need bounds check.
    virtual uint8_t* getPixelAddress(int x, int y) const = 0;
    virtual color_t getPixel(int x, int y) const = 0;
    virtual void putPixel(int x, int y, color_t color) = 0;
    virtual void clear(color_t color) = 0;
    virtual void copy(const Image* src, gfx::Clip area) = 0;
    virtual void drawHLine(int x1, int y, int x2, color_t color) = 0;
    virtual void fillRect(int x1, int y1, int x2, int y2, color_t color) = 0;
    virtual void blendRect(int x1, int y1, int x2, int y2, color_t color, int opacity) = 0;

  protected:
    Image(const ImageSpec& spec);

  private:
    ImageSpec m_spec;
  };

} // namespace doc

// It's here because it needs a complete definition of Image class,
// and then ImageTraits are used in the next functions below.
#include "doc/image_traits.h"

namespace doc {

  inline int calculate_rowstride_bytes(PixelFormat pixelFormat, int pixels_per_row)
  {
    switch (pixelFormat) {
      case IMAGE_RGB:       return RgbTraits::getRowStrideBytes(pixels_per_row);
      case IMAGE_GRAYSCALE: return GrayscaleTraits::getRowStrideBytes(pixels_per_row);
      case IMAGE_INDEXED:   return IndexedTraits::getRowStrideBytes(pixels_per_row);
      case IMAGE_BITMAP:    return BitmapTraits::getRowStrideBytes(pixels_per_row);
    }
    return 0;
  }

} // namespace doc

#endif
