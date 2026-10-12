// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/serial_span.h"

namespace app {

serial_span::~serial_span()
{
  if (m_buf && m_size)
    delete m_buf;
}

void serial_span::init_with_new_buffer()
{
  m_buf = new base::buffer;
  m_size = 1; // We own the buffer
}

void serial_span::init_with_buffer_view(base::buffer& buf)
{
  m_buf = &buf;
  m_size = 0; // We don't own the buffer
}

void serial_span::init_copying_stringstream(std::stringstream& ss)
{
  const std::string str = ss.str();
  m_size = str.size();
  m_buf = new base::buffer(m_size);
  std::copy((const uint8_t*)str.data(), (const uint8_t*)str.data() + m_size, m_buf->data());
}

void serial_span::copy_to_buffer(const base::buffer& src)
{
  ASSERT(m_buf);
  m_buf->resize(src.size());
  std::copy(src.begin(), src.end(), m_buf->begin());
}

} // namespace app
