// Aseprite UI Library
// Copyright (C) 2019-2021  Igara Studio S.A.
// Copyright (C) 2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_FIT_BOUNDS_H_INCLUDED
#define UI_FIT_BOUNDS_H_INCLUDED
#pragma once

#include "gfx/fwd.h"

#include <functional>

namespace ui {

class Display;
class Widget;
class Window;

int fit_bounds(Display* display, int arrowAlign, const gfx::Rect& target, gfx::Rect& bounds);

// Fits a possible rectangle for the given window fitting it with a
// special logic. It works for both cases:
// 1. With multiple windows (so the limit is the parentDisplay screen workarea)
// 2. Or with one window (so the limit is the just display area)
//
// The getWidgetBounds() can be used to get other widgets positions
// in the "fitLogic" (the bounds will be relative to the screen or
// to the display depending if get_multiple_displays() is true or
// false).
void fit_bounds(
  const Display* parentDisplay,
  Window* window,
  const gfx::Rect& candidateBoundsRelativeToParentDisplay,
  std::function<void(const gfx::Rect& workarea,
                     gfx::Rect& bounds,
                     std::function<gfx::Rect(Widget*)> getWidgetBounds)> fitLogic = nullptr);

// The "frame" is a native windows frame bounds.
void limit_with_workarea(Display* parentDisplay, gfx::Rect& frame);

} // namespace ui

#endif
