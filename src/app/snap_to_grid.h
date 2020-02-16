// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SNAP_TO_GRID_H_INCLUDED
#define APP_SNAP_TO_GRID_H_INCLUDED
#pragma once

#include "gfx/fwd.h"

namespace app {

  enum class PreferSnapTo {
    ClosestGridVertex,
    BoxOrigin, BoxEnd,
    FloorGrid, CeilGrid,
  };

  gfx::Point snap_to_grid(const gfx::Rect& grid,
                          const gfx::Point& point,
                          const PreferSnapTo prefer);

} // namespace app

#endif
