// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SERIAL_SPAN_H_INCLUDED
#define APP_SERIAL_SPAN_H_INCLUDED
#pragma once

#include "base/buffer.h"
#include "base/ints.h"

#include <algorithm>
#include <sstream>

namespace app {

// Read-write span/buffer. Acts like a std::span for reading purposes,
// acts like a base::buffer for writing purposes.
class serial_span {
public:
  serial_span() {}
  serial_span(const uint8_t* data, size_t n) : m_data(data), m_size(n) {}
  serial_span(const serial_span&) = delete;
  serial_span& operator=(const serial_span&) = delete;
  serial_span& operator=(serial_span&& other) = delete;
  ~serial_span();
  void init_with_new_buffer();
  void init_with_buffer_view(base::buffer& buf);
  void init_copying_stringstream(std::stringstream& ss);

  void copy_to_buffer(const base::buffer& src);

  const uint8_t* data() const { return m_buf ? m_buf->data() : m_data; }
  size_t size() const { return m_buf ? m_buf->size() : m_size; }

private:
  const uint8_t* m_data = nullptr;
  size_t m_size = 0;
  base::buffer* m_buf = nullptr;
};

} // namespace app

#endif
