// Aseprite Document IO Library
// Copyright (c) 2018 Igara Studio S.A.
// Copyright (c) 2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "dio/decoder.h"

#include "dio/file_interface.h"
#include "doc/document.h"

namespace dio {

Decoder::Decoder()
  : m_delegate(nullptr)
  , m_f(nullptr)
{
}

Decoder::~Decoder()
{
}

void Decoder::initialize(DecodeDelegate* delegate, FileInterface* f)
{
  m_delegate = delegate;
  m_f = f;
}

uint8_t Decoder::read8()
{
  return m_f->read8();
}

uint16_t Decoder::read16()
{
  int b1 = m_f->read8();
  int b2 = m_f->read8();

  if (m_f->ok()) {
    return ((b2 << 8) | b1); // Little endian
  }
  else
    return 0;
}

uint32_t Decoder::read32()
{
  int b1 = m_f->read8();
  int b2 = m_f->read8();
  int b3 = m_f->read8();
  int b4 = m_f->read8();

  if (m_f->ok()) {
    // Little endian
    return ((b4 << 24) | (b3 << 16) | (b2 << 8) | b1);
  }
  else
    return 0;
}

size_t Decoder::readBytes(uint8_t* buf, size_t n)
{
  return m_f->readBytes(buf, n);
}

} // namespace dio
