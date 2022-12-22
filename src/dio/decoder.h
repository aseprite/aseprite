// Aseprite Document IO Library
// Copyright (c) 2018 Igara Studio S.A.
// Copyright (c) 2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DIO_DECODER_H_INCLUDED
#define DIO_DECODER_H_INCLUDED
#pragma once

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
  Decoder();
  virtual ~Decoder();
  virtual void initialize(DecodeDelegate* delegate, FileInterface* f);
  virtual bool decode() = 0;

protected:
  DecodeDelegate* delegate() { return m_delegate; }
  FileInterface* f() { return m_f; }

  uint8_t read8();
  uint16_t read16();
  uint32_t read32();
  uint64_t read64();
  size_t readBytes(uint8_t* buf, size_t n);

private:
  DecodeDelegate* m_delegate;
  FileInterface* m_f;
};

} // namespace dio

#endif
