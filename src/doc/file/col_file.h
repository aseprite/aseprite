// Aseprite Document Library
// Copyright (c) 2022 Igara Studio S.A.
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_FILE_COL_FILE_H_INCLUDED
#define DOC_FILE_COL_FILE_H_INCLUDED
#pragma once

#include <memory>

namespace doc {

class Palette;

namespace file {

std::unique_ptr<Palette> load_col_file(const char* filename);
bool save_col_file(const Palette* pal, const char* filename);

} // namespace file

} // namespace doc

#endif
