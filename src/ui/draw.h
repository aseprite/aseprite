// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_DRAW_H_INCLUDED
#define UI_DRAW_H_INCLUDED
#pragma once

#include "gfx/region.h"

namespace ui { // TODO all these functions are deprecated and must be replaced by Graphics methods.

  void _move_region(const gfx::Region& region, int dx, int dy);

} // namespace ui

#endif
