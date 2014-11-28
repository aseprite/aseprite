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

#ifndef RASTER_IMAGE_BUFFER_H_INCLUDED
#define RASTER_IMAGE_BUFFER_H_INCLUDED
#pragma once

#include "base/shared_ptr.h"

#include <vector>

namespace raster {

  class ImageBuffer {
  public:
    ImageBuffer(size_t size = 1) : m_buffer(size) {
    }

    size_t size() const { return m_buffer.size(); }
    uint8_t* buffer() { return &m_buffer[0]; }

    void resizeIfNecessary(size_t size) {
      if (size > m_buffer.size())
        m_buffer.resize(size);
    }

  private:
    std::vector<uint8_t> m_buffer;
  };

  typedef SharedPtr<ImageBuffer> ImageBufferPtr;

} // namespace raster

#endif
