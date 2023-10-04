// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_TILE_FLAGS_UTILS_H_INCLUDED
#define APP_TILE_FLAGS_UTILS_H_INCLUDED
#pragma once

#include "doc/tile.h"

#include <string>

namespace app {

  void build_tile_flags_string(const doc::tile_flags tf,
                               std::string& result);

} // namespace app

#endif
