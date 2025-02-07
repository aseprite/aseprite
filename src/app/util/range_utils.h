// Aseprite
// Copyright (C) 2020-2021  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UTIL_RANGE_UTILS_H_INCLUDED
#define APP_UTIL_RANGE_UTILS_H_INCLUDED
#pragma once

#include "doc/cel_list.h"

#include <vector>

namespace doc {
class Sprite;
}

namespace app {
using namespace doc;

class DocRange;

doc::CelList get_cels(const doc::Sprite* sprite, const DocRange& range);
doc::CelList get_unique_cels(const doc::Sprite* sprite, const DocRange& range);
doc::CelList get_unique_cels_to_edit_pixels(const doc::Sprite* sprite, const DocRange& range);
doc::CelList get_unique_cels_to_move_cel(const doc::Sprite* sprite, const DocRange& range);

} // namespace app

#endif
