// ASE base library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "base/serialization.h"

#include <iostream>

namespace base {
namespace serialization {

std::ostream& write8(std::ostream& os, uint8_t byte)
{
  os.put(byte);
  return os;
}

uint8_t read8(std::istream& is)
{
  return (uint8_t)is.get();
}

std::ostream& little_endian::write16(std::ostream& os, uint16_t word)
{
  os.put((int)((word & 0x00ff)));
  os.put((int)((word & 0xff00) >> 8));
  return os;
}

std::ostream& little_endian::write32(std::ostream& os, uint32_t dword)
{
  os.put((int)((dword & 0x000000ffl)));
  os.put((int)((dword & 0x0000ff00l) >> 8));
  os.put((int)((dword & 0x00ff0000l) >> 16));
  os.put((int)((dword & 0xff000000l) >> 24));
  return os;
}

uint16_t little_endian::read16(std::istream& is)
{
  int b1, b2;
  b1 = is.get();
  b2 = is.get();
  return ((b2 << 8) | b1);
}

uint32_t little_endian::read32(std::istream& is)
{
  int b1, b2, b3, b4;
  b1 = is.get();
  b2 = is.get();
  b3 = is.get();
  b4 = is.get();
  return ((b4 << 24) | (b3 << 16) | (b2 << 8) | b1);
}

std::ostream& big_endian::write16(std::ostream& os, uint16_t word)
{
  os.put((int)((word & 0xff00) >> 8));
  os.put((int)((word & 0x00ff)));
  return os;
}

std::ostream& big_endian::write32(std::ostream& os, uint32_t dword)
{
  os.put((int)((dword & 0xff000000l) >> 24));
  os.put((int)((dword & 0x00ff0000l) >> 16));
  os.put((int)((dword & 0x0000ff00l) >> 8));
  os.put((int)((dword & 0x000000ffl)));
  return os;
}

uint16_t big_endian::read16(std::istream& is)
{
  int b1, b2;
  b2 = is.get();
  b1 = is.get();
  return ((b2 << 8) | b1);
}

uint32_t big_endian::read32(std::istream& is)
{
  int b1, b2, b3, b4;
  b4 = is.get();
  b3 = is.get();
  b2 = is.get();
  b1 = is.get();
  return ((b4 << 24) | (b3 << 16) | (b2 << 8) | b1);
}

} // namespace serialization
} // namespace base
