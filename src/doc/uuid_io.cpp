// Aseprite Document Library
// Copyright (c) 2025 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "doc/uuid_io.h"

#include "base/serialization.h"
#include "base/uuid.h"

namespace doc {

using namespace base::serialization;

base::Uuid read_uuid(std::istream& is)
{
  base::Uuid value;
  uint8_t* bytes = value.bytes();
  for (int i = 0; i < 16; ++i) {
    bytes[i] = read8(is);
  }
  return value;
}

void write_uuid(std::ostream& os, const base::Uuid& uuid)
{
  for (int i = 0; i < 16; ++i) {
    write8(os, uuid[i]);
  }
}

} // namespace doc
