// Aseprite UI Library
// Copyright (C) 2020-2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_SCROLL_WINDOW_H_INCLUDED
#define UI_SCROLL_WINDOW_H_INCLUDED
#pragma once

#include "gfx/rect.h"

namespace ui {

class View;
class Window;

enum class AddScrollBarsOption {
  IfNeeded,
  Always,
};

// Adds a view to wrap the window's first non-decorative child so if
// the window is resized/too small we can scroll the content.
View* add_scrollbars(Window* window,
                     const gfx::Rect& workarea,
                     gfx::Rect& bounds,
                     AddScrollBarsOption option);

} // namespace ui

#endif
