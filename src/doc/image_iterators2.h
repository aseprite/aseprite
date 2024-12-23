// Aseprite Document Library
// Copyright (c) 2024 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_IMAGE_ITERATOR2_H_INCLUDED
#define DOC_IMAGE_ITERATOR2_H_INCLUDED
#pragma once

#include "base/ints.h"
#include "gfx/fwd.h"

namespace doc {

class Image;

// New iterators classes.

enum class IteratorStart : uint8_t { TopLeft, TopRight, BottomLeft, BottomRight };

class ReadIterator {
public:
  ReadIterator(const Image* image,
               const gfx::Rect& bounds,
               IteratorStart start = IteratorStart::TopLeft);

  const uint8_t* addr8() const { return m_addr; }
  const uint16_t* addr16() const { return (uint16_t*)m_addr; }
  const uint32_t* addr32() const { return (uint32_t*)m_addr; }

  template<typename ImageTraits>
  typename ImageTraits::const_address_t addr() const
  {
    return (typename ImageTraits::const_address_t)m_addr;
  }

  bool nextLine()
  {
    m_addr += m_nextRow;
    return (m_rows-- > 0);
  }

protected:
  uint8_t* m_addr = nullptr;

private:
  int m_rows = 0;
  int m_nextRow = 0;
};

class WriteIterator : public ReadIterator {
public:
  WriteIterator(Image* image,
                const gfx::Rect& bounds,
                const IteratorStart start = IteratorStart::TopLeft)
    : ReadIterator(image, bounds, start)
  {
  }

  uint8_t* addr8() { return m_addr; }
  uint16_t* addr16() { return (uint16_t*)m_addr; }
  uint32_t* addr32() { return (uint32_t*)m_addr; }

  template<typename ImageTraits>
  uint32_t* addr()
  {
    return (typename ImageTraits::address_t)m_addr;
  }
};

} // namespace doc

#endif
