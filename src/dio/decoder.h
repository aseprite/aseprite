// Aseprite Document IO Library
// Copyright (c) 2018-2026 Igara Studio S.A.
// Copyright (c) 2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DIO_DECODER_H_INCLUDED
#define DIO_DECODER_H_INCLUDED
#pragma once

#include "dio/file_interface.h"

#include <cstdint>
#include <cstring>

namespace doc {
class Document;
}

namespace dio {

class DecodeDelegate;
class FileInterface;

class Decoder {
public:
  virtual ~Decoder() {}

  virtual void initialize(DecodeDelegate* delegate, FileInterface* f)
  {
    m_delegate = delegate;
    m_f = f;
  }

  virtual bool decode() = 0;

protected:
  DecodeDelegate* delegate() { return m_delegate; }
  FileInterface* f() { return m_f; }

  size_t tell() { return m_f->tell(); }
  void seek(size_t absPos) { m_f->seek(absPos); }

  uint8_t read8() { return m_f->read8(); }

  uint16_t read16()
  {
    const int b1 = read8();
    const int b2 = read8();

    if (m_f->ok())
      return ((b2 << 8) | b1); // Little endian
    return 0;
  }

  uint32_t read32()
  {
    const int b1 = read8();
    const int b2 = read8();
    const int b3 = read8();
    const int b4 = read8();

    // Little endian
    if (m_f->ok())
      return ((b4 << 24) | (b3 << 16) | (b2 << 8) | b1);
    return 0;
  }

  uint64_t read64()
  {
    const int b1 = read8();
    const int b2 = read8();
    const int b3 = read8();
    const int b4 = read8();
    const int b5 = read8();
    const int b6 = read8();
    const int b7 = read8();
    const int b8 = read8();

    // Little endian
    if (m_f->ok()) {
      return (((long long)b8 << 56) | ((long long)b7 << 48) | ((long long)b6 << 40) |
              ((long long)b5 << 32) | ((long long)b4 << 24) | ((long long)b3 << 16) |
              ((long long)b2 << 8) | (long long)b1);
    }
    return 0;
  }

  size_t readBytes(uint8_t* buf, size_t n) { return m_f->readBytes(buf, n); }

private:
  DecodeDelegate* m_delegate = nullptr;
  FileInterface* m_f = nullptr;
};

} // namespace dio

#endif
