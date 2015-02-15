// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_SNAP_TO_GRID_H_INCLUDED
#define APP_SNAP_TO_GRID_H_INCLUDED
#pragma once

#include "gfx/fwd.h"

namespace app {

  gfx::Point snap_to_grid(const gfx::Rect& grid, const gfx::Point& point);

} // namespace app

#endif
