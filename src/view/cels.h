// Aseprite View Library
// Copyright (C) 2020-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef VIEW_CELS_H_INCLUDED
#define VIEW_CELS_H_INCLUDED
#pragma once

#include "doc/cel_list.h"

namespace doc {
  class Sprite;
}

namespace view {
  class Range;

  doc::CelList get_cels(const doc::Sprite* sprite, const Range& range);
  doc::CelList get_unique_cels(const doc::Sprite* sprite, const Range& range);
  doc::CelList get_unique_cels_to_edit_pixels(const doc::Sprite* sprite, const Range& range);
  doc::CelList get_unique_cels_to_move_cel(const doc::Sprite* sprite, const Range& range);

} // namespace view

#endif  // VIEW_CELS_H_INCLUDED
