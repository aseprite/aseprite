// Aseprite UI Library
// Copyright (C) 2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_POINT_AT_H_INCLUDED
#define UI_POINT_AT_H_INCLUDED
#pragma once

#include "gfx/fwd.h"

namespace ui {

  int fit_bounds(int arrowAlign, const gfx::Rect& target, gfx::Rect& bounds);

} // namespace ui

#endif
