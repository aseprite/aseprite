// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_MASK_IO_H_INCLUDED
#define DOC_MASK_IO_H_INCLUDED
#pragma once

#include <iosfwd>

namespace doc {

class Mask;

void write_mask(std::ostream& os, const Mask* mask);
Mask* read_mask(std::istream& is);

} // namespace doc

#endif
