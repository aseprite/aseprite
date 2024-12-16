// Aseprite Document IO Library
// Copyright (c) 2017-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DIO_FILE_INTERFACE_H_INCLUDED
#define DIO_FILE_INTERFACE_H_INCLUDED
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace dio {

class FileInterface {
public:
  virtual ~FileInterface() {}

  // Returns true if we can read/write bytes from/into the file
  virtual bool ok() const = 0;

  // Current position in the file
  virtual size_t tell() = 0;

  // Jump to the given position in the file
  virtual void seek(size_t absPos) = 0;

  // Returns the next byte in the file or 0 if ok() = false
  virtual uint8_t read8() = 0;
  virtual size_t readBytes(uint8_t* buf, size_t n) = 0;

  // Writes one byte in the file (or do nothing if ok() = false)
  virtual void write8(uint8_t value) = 0;
};

class StdioFileInterface : public FileInterface {
public:
  StdioFileInterface(FILE* file);
  bool ok() const override;
  size_t tell() override;
  void seek(size_t absPos) override;
  uint8_t read8() override;
  size_t readBytes(uint8_t* buf, size_t n) override;
  void write8(uint8_t value) override;

private:
  FILE* m_file;
  bool m_ok;
};

} // namespace dio

#endif
