// Aseprite UI Library
// Copyright (C) 2001-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_SCROLL_HELPER_H_INCLUDED
#define UI_SCROLL_HELPER_H_INCLUDED
#pragma once

#include "gfx/rect.h"
#include "gfx/size.h"

namespace ui {

  class ScrollBar;

  void setup_scrollbars(const gfx::Size& scrollableSize,
                        gfx::Rect& viewportArea,
                        Widget& parent,
                        ScrollBar& hbar,
                        ScrollBar& vbar);

} // namespace ui

#endif
