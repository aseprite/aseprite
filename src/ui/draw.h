// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_DRAW_H_INCLUDED
#define UI_DRAW_H_INCLUDED
#pragma once

#include "gfx/rect.h"
#include "gfx/region.h"
#include "ui/base.h"
#include "ui/color.h"

struct FONT;
struct BITMAP;

// TODO all these functions are deprecated and must be replaced by Graphics methods.

namespace ui {

  void _draw_text(BITMAP* bmp, FONT* f, const char* text, int x, int y, ui::Color fg, ui::Color bg, bool fill_bg, int underline_height = 1);
  void _move_region(const gfx::Region& region, int dx, int dy);

} // namespace ui

#endif
