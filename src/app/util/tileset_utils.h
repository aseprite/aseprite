// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_TILESET_UTILS_H_INCLUDED
#define APP_TILESET_UTILS_H_INCLUDED
#pragma once

#include "doc/tileset.h"

#include <string>

namespace app {

// Builds a string representation of a tileset for using in
// labels in the UI.
std::string tileset_label(const doc::Tileset* tileset, doc::tileset_index index);

} // namespace app

#endif
