// Aseprite Document Library
// Copyright (c) 2024 Igara Studio S.A.
// Copyright (c) 2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_SLICE_IO_H_INCLUDED
#define DOC_SLICE_IO_H_INCLUDED
#pragma once

#include "doc/serial_format.h"

#include <iosfwd>

namespace doc {

class Slice;
class SliceKey;

void write_slice(std::ostream& os, const Slice* slice);
Slice* read_slice(std::istream& is, bool setId = true, SerialFormat serial = SerialFormat::LastVer);

void write_slicekey(std::ostream& os, const SliceKey& sliceKey);
SliceKey read_slicekey(std::istream& is);

} // namespace doc

#endif
