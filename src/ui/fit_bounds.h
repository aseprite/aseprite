// Aseprite UI Library
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_FIT_BOUNDS_H_INCLUDED
#define UI_FIT_BOUNDS_H_INCLUDED
#pragma once

#include "gfx/fwd.h"

namespace ui {

  class Display;

  int fit_bounds(Display* display, int arrowAlign, const gfx::Rect& target, gfx::Rect& bounds);

} // namespace ui

#endif
