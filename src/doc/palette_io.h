// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_PALETTE_IO_H_INCLUDED
#define DOC_PALETTE_IO_H_INCLUDED
#pragma once

#include <iosfwd>

namespace doc {

class Palette;

void write_palette(std::ostream& os, const Palette* palette);
Palette* read_palette(std::istream& is);

} // namespace doc

#endif
