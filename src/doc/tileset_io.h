// Aseprite Document Library
// Copyright (C) 2019-2020  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_TILESET_IO_H_INCLUDED
#define DOC_TILESET_IO_H_INCLUDED
#pragma once

#include "app/crash/doc_format.h"
#include "base/ints.h"

#include <iosfwd>

// Extra BYTE with special flags to check the tileset version.  This
// field didn't exist in Aseprite v1.3-alpha3 (so read8() fails = 0)
#define TILESET_VER1     1

// Tileset has UserData now
#define TILESET_VER2     2

// Tileset name (was missing originally) + each tileset's tile has
// UserData now
#define TILESET_VER3     3

namespace doc {

  class CancelIO;
  class Sprite;
  class Tileset;

  bool write_tileset(std::ostream& os,
                     const Tileset* tileset,
                     CancelIO* cancel = nullptr);

  Tileset* read_tileset(std::istream& is,
                        Sprite* sprite,
                        bool setId = true,
                        uint32_t* tilesetVer = nullptr,
                        const int docFormatVer = DOC_FORMAT_VERSION_LAST);

} // namespace doc

#endif
