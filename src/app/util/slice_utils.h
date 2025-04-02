// Aseprite
// Copyright (C) 2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SLICE_UTILS_H_INCLUDED
#define APP_SLICE_UTILS_H_INCLUDED
#pragma once

#include <string>
#include <vector>

namespace doc {
class Slice;
class Sprite;
} // namespace doc

namespace app {

class Site;

std::string get_unique_slice_name(const doc::Sprite* sprite,
                                  const std::string& namePrefix = std::string());

std::vector<doc::Slice*> get_selected_slices(const Site& site);

} // namespace app

#endif
