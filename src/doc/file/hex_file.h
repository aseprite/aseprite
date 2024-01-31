// Aseprite Document Library
// Copyright (c) 2022-2024 Igara Studio S.A.
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_FILE_HEX_FILE_H_INCLUDED
#define DOC_FILE_HEX_FILE_H_INCLUDED
#pragma once

#include <iosfwd>
#include <memory>

namespace doc {

  class Palette;
  class PalettePicks;

  namespace file {

    std::unique_ptr<Palette> load_hex_file(const char* filename);
    bool save_hex_file(const Palette* pal, const char* filename);
    void save_hex_file(const Palette* palette,
                       const PalettePicks* picks,
                       const bool include_hash_char,
                       const bool final_eol,
                       std::ostream& f);

  } // namespace file
} // namespace doc

#endif
