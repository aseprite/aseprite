// Aseprite Document Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/string_io.h"

#include "base/serialization.h"

#include <iostream>
#include <vector>

namespace doc {

using namespace base::serialization;
using namespace base::serialization::little_endian;

void write_string(std::ostream& os, const std::string& str)
{
  write16(os, (uint16_t)str.size());
  if (!str.empty())
    os.write(str.c_str(), str.size());
}

std::string read_string(std::istream& is)
{
  uint16_t length = read16(is);
  std::vector<char> str(length + 1);
  if (length > 0) {
    is.read(&str[0], length);
    str[length] = 0;
  }
  else
    str[0] = 0;

  return std::string(&str[0]);
}

} // namespace doc
