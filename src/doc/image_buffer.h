// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_IMAGE_BUFFER_H_INCLUDED
#define DOC_IMAGE_BUFFER_H_INCLUDED
#pragma once

#include "base/shared_ptr.h"

#include <vector>

namespace doc {

  class ImageBuffer {
  public:
    ImageBuffer(size_t size) : m_buffer(size) {
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

} // namespace doc

#endif
