// Aseprite Document Library
// Copyright (c) 2025 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_UUID_IO_H_INCLUDED
#define DOC_UUID_IO_H_INCLUDED
#pragma once

#include "base/uuid.h"

#include <iosfwd>

namespace doc {

base::Uuid read_uuid(std::istream& is);

void write_uuid(std::ostream& os, const base::Uuid& uuid);

} // namespace doc

#endif
