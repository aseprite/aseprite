// Aseprite Document Library
// Copyright (C) 2019-2024  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_IMAGE_BUFFER_H_INCLUDED
#define DOC_IMAGE_BUFFER_H_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "base/ints.h"
#include "doc/aligned_memory.h"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <memory>

namespace doc {

class ImageBuffer {
public:
  ImageBuffer(std::size_t size = 1)
    : m_size(doc_align_size(size))
    , m_buffer((uint8_t*)doc_aligned_alloc(m_size))
  {
    if (!m_buffer)
      throw std::bad_alloc();
  }

  ~ImageBuffer() noexcept
  {
    if (m_buffer)
      doc_aligned_free(m_buffer);
  }

  std::size_t size() const { return m_size; }
  uint8_t* buffer() { return (uint8_t*)m_buffer; }

  void resizeIfNecessary(std::size_t size)
  {
    if (size > m_size) {
      if (m_buffer) {
        doc_aligned_free(m_buffer);
        m_buffer = nullptr;
      }

      m_size = doc_align_size(size);
      m_buffer = (uint8_t*)doc_aligned_alloc(m_size);
      if (!m_buffer)
        throw std::bad_alloc();
    }
  }

private:
  size_t m_size;
  uint8_t* m_buffer;

  DISABLE_COPYING(ImageBuffer);
};

using ImageBufferPtr = std::shared_ptr<ImageBuffer>;

} // namespace doc

#endif
