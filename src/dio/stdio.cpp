// Aseprite Document IO Library
// Copyright (c) 2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "dio/file_interface.h"

namespace dio {

StdioFileInterface::StdioFileInterface(FILE* file) : m_file(file), m_ok(true)
{
}

bool StdioFileInterface::ok() const
{
  return m_ok;
}

size_t StdioFileInterface::tell()
{
  return ftell(m_file);
}

void StdioFileInterface::seek(size_t absPos)
{
  fseek(m_file, absPos, SEEK_SET);
}

uint8_t StdioFileInterface::read8()
{
  int value = fgetc(m_file);
  if (value != EOF)
    return value;

  m_ok = false;
  return 0;
}

size_t StdioFileInterface::readBytes(uint8_t* buf, size_t n)
{
  int n2 = fread(buf, 1, n, m_file);
  if (n2 != n)
    m_ok = false;
  return n2;
}

void StdioFileInterface::write8(uint8_t value)
{
  fputc(value, m_file);
}

} // namespace dio
