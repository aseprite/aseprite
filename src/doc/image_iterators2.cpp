// Aseprite Document Library
// Copyright (c) 2024 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/image.h"

namespace doc {

ReadIterator::ReadIterator(const Image* image, const gfx::Rect& bounds, const IteratorStart start)
  : m_rows(bounds.h)
{
  switch (start) {
    case IteratorStart::TopLeft:
      m_addr = image->getPixelAddress(bounds.x, bounds.y);
      m_nextRow = image->rowBytes();
      break;
    case IteratorStart::TopRight:
      m_addr = image->getPixelAddress(bounds.x2() - 1, bounds.y);
      m_nextRow = image->rowBytes();
      break;
    case IteratorStart::BottomLeft:
      m_addr = image->getPixelAddress(bounds.x, bounds.y2() - 1);
      m_nextRow = -image->rowBytes();
      break;
    case IteratorStart::BottomRight:
      m_addr = image->getPixelAddress(bounds.x2() - 1, bounds.y2() - 1);
      m_nextRow = -image->rowBytes();
      break;
  }
  m_addr -= m_nextRow; // This is canceled in the first nextLine() call.
}

} // namespace doc
