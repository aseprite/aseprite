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

#ifndef RASTER_IMAGE_ITERATOR_H_INCLUDED
#define RASTER_IMAGE_ITERATOR_H_INCLUDED

#ifndef NDEBUG
#define RASTER_DEBUG_ITERATORS
#endif

#include "gfx/rect.h"
#include "raster/color.h"
#include "raster/image.h"
#include "raster/image_traits.h"

#ifdef RASTER_DEBUG_ITERATORS
#include "gfx/point.h"
#endif

#include <cstdlib>
#include <iterator>

#include <iostream>

namespace raster {

  template<typename ImageTraits,
           typename PointerType,
           typename ReferenceType>
  class ImageIteratorT : public std::iterator<std::forward_iterator_tag,
                                              typename ImageTraits::pixel_t,
                                              ptrdiff_t,
                                              PointerType,
                                              ReferenceType> {
  public:
    // GCC 4.6 needs these re-definitions here.
    typedef ptrdiff_t difference_type;
    typedef PointerType pointer;
    typedef ReferenceType reference;

    ImageIteratorT() : m_ptr(NULL) {
    }

    ImageIteratorT(const ImageIteratorT& other) :
      m_ptr(other.m_ptr),
      m_x(other.m_x),
      m_width(other.m_width),
      m_nextRow(other.m_nextRow)
#ifdef RASTER_DEBUG_ITERATORS
      , m_image(other.m_image)
      , m_X0(other.m_X0), m_X(other.m_X), m_Y(other.m_Y)
#endif
    {
    }

    ImageIteratorT(const Image* image, const gfx::Rect& bounds, int x, int y) :
      m_ptr((pointer)image->getPixelAddress(x, y)),
      m_x(x - bounds.x),
      m_width(bounds.w),
      m_nextRow(image->getWidth() - bounds.w)
#ifdef RASTER_DEBUG_ITERATORS
      , m_image(const_cast<Image*>(image))
      , m_X0(bounds.x)
      , m_X(x), m_Y(y)
#endif
    {
      ASSERT(bounds.contains(gfx::Point(x, y)));
      ASSERT(image->getBounds().contains(bounds));
    }

    ImageIteratorT& operator=(const ImageIteratorT& other) {
      m_ptr = other.m_ptr;
      m_x = other.m_x;
      m_width = other.m_width;
      m_nextRow = other.m_nextRow;
#ifdef RASTER_DEBUG_ITERATORS
      m_image = other.m_image;
      m_X0 = other.m_X0;
      m_X = other.m_X;
      m_Y = other.m_Y;
#endif
      return *this;
    }

    bool operator==(const ImageIteratorT& other) const {
#ifdef RASTER_DEBUG_ITERATORS
      if (m_ptr == other.m_ptr) {
        ASSERT(m_X == other.m_X && m_Y == other.m_Y);
      }
      else {
        ASSERT(m_X != other.m_X || m_Y != other.m_Y);
      }
#endif
      return m_ptr == other.m_ptr;
    }
    bool operator!=(const ImageIteratorT& other) const {
#ifdef RASTER_DEBUG_ITERATORS
      if (m_ptr != other.m_ptr) {
        ASSERT(m_X != other.m_X || m_Y != other.m_Y);
      }
      else {
        ASSERT(m_X == other.m_X && m_Y == other.m_Y);
      }
#endif
      return m_ptr != other.m_ptr;
    }
    bool operator<(const ImageIteratorT& other) const { return m_ptr < other.m_ptr; }
    bool operator>(const ImageIteratorT& other) const { return m_ptr > other.m_ptr; }
    bool operator<=(const ImageIteratorT& other) const { return m_ptr <= other.m_ptr; }
    bool operator>=(const ImageIteratorT& other) const { return m_ptr >= other.m_ptr; }

    ImageIteratorT& operator++() {
#ifdef RASTER_DEBUG_ITERATORS
      ASSERT(m_image->getBounds().contains(gfx::Point(m_X, m_Y)));
#endif

      ++m_ptr;
      ++m_x;

#ifdef RASTER_DEBUG_ITERATORS
      ++m_X;
#endif

      if (m_x == m_width) {
        m_ptr += m_nextRow;
        m_x = 0;

#ifdef RASTER_DEBUG_ITERATORS
        m_X = m_X0;
        ++m_Y;

        if (m_Y < m_image->getHeight()) {
          pointer expected_ptr = (pointer)m_image->getPixelAddress(m_X, m_Y);
          ASSERT(expected_ptr == m_ptr);
        }
#endif
      }
#ifdef RASTER_DEBUG_ITERATORS
      else {
        pointer expected_ptr = (pointer)m_image->getPixelAddress(m_X, m_Y);
        ASSERT(expected_ptr == m_ptr);
      }
#endif

      return *this;
    }

    ImageIteratorT& operator+=(int diff) {
      while (diff-- > 0)
        operator++();
      return *this;
    }

    ImageIteratorT operator++(int) {
      ImageIteratorT tmp(*this);
      operator++();
      return tmp;
    }

    reference operator*() { return *m_ptr; }

  private:
    pointer m_ptr;
    int m_bit;
    int m_x;
    int m_width;
    int m_nextRow;
#ifdef RASTER_DEBUG_ITERATORS // Data used for debugging purposes.
    Image* m_image;
    int m_X0, m_X, m_Y;
#endif
  };

  template<typename ImageTraits>
  class ImageIterator : public ImageIteratorT<ImageTraits,
                                              typename ImageTraits::pixel_t *,
                                              typename ImageTraits::pixel_t&> {
  public:
    // GCC 4.6 needs these re-definitions here.
    typedef typename ImageTraits::pixel_t* pointer;
    typedef typename ImageTraits::pixel_t& reference;

    ImageIterator() {
    }

    ImageIterator(const Image* image, const gfx::Rect& bounds, int x, int y) :
      ImageIteratorT<ImageTraits,
                     pointer,
                     reference>(image, bounds, x, y) {
    }
  };

  template<typename ImageTraits>
  class ImageConstIterator : public ImageIteratorT<ImageTraits,
                                                   typename ImageTraits::pixel_t const *,
                                                   typename ImageTraits::pixel_t const &> {
  public:
    // GCC 4.6 needs these re-definitions here.
    typedef typename ImageTraits::pixel_t const* pointer;
    typedef typename ImageTraits::pixel_t const& reference;

    ImageConstIterator() {
    }

    ImageConstIterator(const Image* image, const gfx::Rect& bounds, int x, int y) :
      ImageIteratorT<ImageTraits,
                     pointer,
                     reference>(image, bounds, x, y) {
    }
  };

  //////////////////////////////////////////////////////////////////////
  // Iterator for BitmapTraits

  class BitPixelAccess {
  public:
    BitPixelAccess() :
      m_ptr(NULL),
      m_bit(0) {
    }

    void reset(BitmapTraits::address_t ptr, int bit) {
      m_ptr = ptr;
      m_bit = bit;
    }

    void reset(BitmapTraits::pixel_t const* ptr, int bit) {
      m_ptr = const_cast<BitmapTraits::address_t>(ptr);
      m_bit = bit;
    }

    operator color_t() const {
      return (*m_ptr & m_bit) ? 1: 0;
    }

    BitPixelAccess& operator=(color_t value) {
      if (value)
        *m_ptr |= m_bit;
      else
        *m_ptr &= ~m_bit;
      return *this;
    }

    // It doesn't copy the BitPixelAccess, it must copy the bit from
    // "other" to "this".
    BitPixelAccess& operator=(const BitPixelAccess& other) {
      return this->operator=((color_t)other);
    }

    bool operator==(int b) const {
      return (color_t(*this) == b);
    }

    bool operator==(color_t b) const {
      return (color_t(*this) == b);
    }

    bool operator==(const BitPixelAccess& b) const {
      return (color_t(*this) == color_t(b));
    }

    bool operator!=(int b) const {
      return (color_t(*this) != b);
    }

    bool operator!=(color_t b) const {
      return (color_t(*this) != b);
    }

    bool operator!=(const BitPixelAccess& b) const {
      return (color_t(*this) != color_t(b));
    }

  private:
    // Non-copyable by copy constructor.
    BitPixelAccess(const BitPixelAccess& other);

    BitmapTraits::address_t m_ptr;
    int m_bit;
  };

  inline bool operator==(int a, const BitPixelAccess& b) {
    return (a == color_t(b));
  }

  inline bool operator==(color_t a, const BitPixelAccess& b) {
    return (a == color_t(b));
  }

  inline bool operator!=(int a, const BitPixelAccess& b) {
    return (a != color_t(b));
  }

  inline bool operator!=(color_t a, const BitPixelAccess& b) {
    return (a != color_t(b));
  }

  template<typename PointerType,
           typename ReferenceType>
  class ImageIteratorT<BitmapTraits, PointerType, ReferenceType>
    : public std::iterator<std::forward_iterator_tag,
                           BitmapTraits::pixel_t,
                           ptrdiff_t,
                           PointerType,
                           ReferenceType> {
  public:
    // GCC 4.6 needs these re-definitions here.
    typedef ptrdiff_t difference_type;
    typedef PointerType pointer;
    typedef ReferenceType reference;
    enum { pixels_per_byte = BitmapTraits::pixels_per_byte };

    ImageIteratorT() : m_ptr(NULL) {
    }

    ImageIteratorT(const ImageIteratorT& other) :
      m_ptr(other.m_ptr),
      m_x(other.m_x),
      m_subPixel(other.m_subPixel),
      m_subPixel0(other.m_subPixel0),
      m_width(other.m_width),
      m_nextRow(other.m_nextRow)
#ifdef RASTER_DEBUG_ITERATORS
      , m_image(other.m_image)
      , m_X0(other.m_X0), m_X(other.m_X), m_Y(other.m_Y)
#endif
    {
    }

    ImageIteratorT(const Image* image, const gfx::Rect& bounds, int x, int y) :
      m_ptr((pointer)image->getPixelAddress(x, y)),
      m_x(x - bounds.x),
      m_subPixel(x % 8),
      m_subPixel0(bounds.x % 8),
      m_width(bounds.w),
      m_nextRow(// Bytes on the right of this row to jump to the beginning of the next one
                BitmapTraits::getRowStrideBytes((pixels_per_byte*BitmapTraits::getRowStrideBytes(image->getWidth()))
                                                - (bounds.x+bounds.w))
                // Bytes on the left of the next row to go to bounds.x byte
                + bounds.x/pixels_per_byte)

#ifdef RASTER_DEBUG_ITERATORS
      , m_image(const_cast<Image*>(image))
      , m_X0(bounds.x)
      , m_X(x), m_Y(y)
#endif
    {
      ASSERT(bounds.contains(gfx::Point(x, y)));
    }

    ImageIteratorT& operator=(const ImageIteratorT& other) {
      m_ptr = other.m_ptr;
      m_x = other.m_x;
      m_subPixel = other.m_subPixel;
      m_subPixel0 = other.m_subPixel0;
      m_width = other.m_width;
      m_nextRow = other.m_nextRow;
#ifdef RASTER_DEBUG_ITERATORS
      m_image = other.m_image;
      m_X0 = other.m_X0;
      m_X = other.m_X;
      m_Y = other.m_Y;
#endif
      return *this;
    }

    bool operator==(const ImageIteratorT& other) const {
#ifdef RASTER_DEBUG_ITERATORS
      if (m_ptr == other.m_ptr &&
          m_subPixel == other.m_subPixel) {
        ASSERT(m_X == other.m_X && m_Y == other.m_Y);
      }
      else {
        ASSERT(m_X != other.m_X || m_Y != other.m_Y);
      }
#endif
      return m_ptr == other.m_ptr;
    }
    bool operator!=(const ImageIteratorT& other) const {
#ifdef RASTER_DEBUG_ITERATORS
      if (m_ptr != other.m_ptr ||
          m_subPixel != other.m_subPixel) {
        ASSERT(m_X != other.m_X || m_Y != other.m_Y);
      }
      else {
        ASSERT(m_X == other.m_X && m_Y == other.m_Y);
      }
#endif
      return m_ptr != other.m_ptr;
    }

    ImageIteratorT& operator++() {
#ifdef RASTER_DEBUG_ITERATORS
      ASSERT(m_image->getBounds().contains(gfx::Point(m_X, m_Y)));
#endif

      ++m_x;
      ++m_subPixel;

      if (m_subPixel == 8) {
        m_subPixel = 0;
        ++m_ptr;
      }

#ifdef RASTER_DEBUG_ITERATORS
      ++m_X;
#endif

      if (m_x == m_width) {
        m_ptr += m_nextRow;
        m_x = 0;
        m_subPixel = m_subPixel0;

#ifdef RASTER_DEBUG_ITERATORS
        m_X = m_X0;
        ++m_Y;

        if (m_Y < m_image->getHeight()) {
          pointer expected_ptr = (pointer)m_image->getPixelAddress(m_X, m_Y);
          ASSERT(expected_ptr == m_ptr);
          ASSERT(m_subPixel == m_X % 8);
        }
#endif
      }
#ifdef RASTER_DEBUG_ITERATORS
      else {
        pointer expected_ptr = (pointer)m_image->getPixelAddress(m_X, m_Y);
        ASSERT(expected_ptr == m_ptr);
        ASSERT(m_subPixel == m_X % 8);
      }
#endif

      return *this;
    }

    ImageIteratorT& operator+=(int diff) {
      while (diff-- > 0)
        operator++();
      return *this;
    }

    ImageIteratorT operator++(int) {
      ImageIteratorT tmp(*this);
      operator++();
      return tmp;
    }

    reference operator*() const {
      m_access.reset(m_ptr, 1 << m_subPixel);
      return m_access;
    }

    reference operator*() {
      m_access.reset(m_ptr, 1 << m_subPixel);
      return m_access;
    }

  private:
    pointer m_ptr;
    int m_x, m_subPixel, m_subPixel0;
    int m_width;
    int m_nextRow;
    mutable BitPixelAccess m_access;
#ifdef RASTER_DEBUG_ITERATORS // Data used for debugging purposes.
    Image* m_image;
    int m_X0, m_X, m_Y;
#endif
  };

  template<>
  class ImageIterator<BitmapTraits> : public ImageIteratorT<BitmapTraits,
                                                            uint8_t*,
                                                            BitPixelAccess&> {
  public:
    typedef ImageIteratorT<BitmapTraits,
                           uint8_t*,
                           BitPixelAccess&> Base;

    ImageIterator() {
    }

    ImageIterator(const Image* image, const gfx::Rect& bounds, int x, int y) :
      Base(image, bounds, x, y) {
    }
  };

  template<>
  class ImageConstIterator<BitmapTraits> : public ImageIteratorT<BitmapTraits,
                                                                 uint8_t const*,
                                                                 const BitPixelAccess&> {
  public:
    typedef ImageIteratorT<BitmapTraits,
                           uint8_t const*,
                           const BitPixelAccess&> Base;

    ImageConstIterator() {
    }

    ImageConstIterator(const Image* image, const gfx::Rect& bounds, int x, int y) :
      Base(image, bounds, x, y) {
    }
  };

} // namespace raster

#endif
