// Aseprite Document Library
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_IMAGE_BUFFER_H_INCLUDED
#define DOC_IMAGE_BUFFER_H_INCLUDED
#pragma once

#include "base/ints.h"

#include <cstddef>
#include <memory>
#include <vector>

namespace doc {

  class ImageBuffer {
  public:
    ImageBuffer(std::size_t size = 1) : m_buffer(size) {
    }

    std::size_t size() const { return m_buffer.size(); }
    uint8_t* buffer() { return &m_buffer[0]; }

    void resizeIfNecessary(std::size_t size) {
      if (size > m_buffer.size())
        m_buffer.resize(size);
    }

  private:
    std::vector<uint8_t> m_buffer;
  };

  typedef std::shared_ptr<ImageBuffer> ImageBufferPtr;

} // namespace doc

#endif
