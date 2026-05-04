// Aseprite Document IO Library
// Copyright (c) 2026 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DIO_ENCODER_H_INCLUDED
#define DIO_ENCODER_H_INCLUDED
#pragma once

#include <cstdint>
#include <cstring>

#include "dio/file_interface.h"
#include "doc/frame.h"

namespace dio {

class EncodeDelegate;
class FileInterface;

class Encoder {
public:
  virtual ~Encoder() {}

  virtual void initialize(EncodeDelegate* delegate, FileInterface* f)
  {
    m_delegate = delegate;
    m_f = f;
  }

  virtual bool encode() = 0;

protected:
  EncodeDelegate* delegate() { return m_delegate; }
  FileInterface* f() { return m_f; }

  size_t tell() { return m_f->tell(); }

  void seek(size_t absPos) { m_f->seek(absPos); }

  void write8(const uint8_t value) { m_f->write8(value); }
  size_t writeBytes(uint8_t* buf, size_t n) { return m_f->writeBytes(buf, n); }

  void write16(const uint16_t value)
  {
    // Little endian
    write8((value & 0x00ff));
    write8((value & 0xff00) >> 8);
  }

  void write32(const uint32_t value)
  {
    // Little endian
    write8((value & 0x000000ff));
    write8((value & 0x0000ff00) >> 8);
    write8((value & 0x00ff0000) >> 16);
    write8((value & 0xff000000) >> 24);
  }

  void write64(const uint64_t value)
  {
    // Little endian
    write8((value & 0x00000000000000ffL));
    write8((value & 0x000000000000ff00L) >> 8);
    write8((value & 0x0000000000ff0000L) >> 16);
    write8((value & 0x00000000ff000000L) >> 24);
    write8((value & 0x000000ff00000000L) >> 32);
    write8((value & 0x0000ff0000000000L) >> 40);
    write8((value & 0x00ff000000000000L) >> 48);
    write8((value & 0xff00000000000000L) >> 56);
  }

private:
  EncodeDelegate* m_delegate = nullptr;
  FileInterface* m_f = nullptr;
};

} // namespace dio

#endif
