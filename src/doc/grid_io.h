// Aseprite Document Library
// Copyright (C) 2019  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_GRID_IO_H_INCLUDED
#define DOC_GRID_IO_H_INCLUDED
#pragma once

#include <iosfwd>

namespace doc {

class Grid;

bool write_grid(std::ostream& os, const Grid& grid);
Grid read_grid(std::istream& is);

} // namespace doc

#endif
