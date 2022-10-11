// Aseprite Document Library
// Copyright (C) 2019-2020  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_TILESET_IO_H_INCLUDED
#define DOC_TILESET_IO_H_INCLUDED
#pragma once

#include <iosfwd>

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
                        bool* isOldVersion = nullptr);

} // namespace doc

#endif
