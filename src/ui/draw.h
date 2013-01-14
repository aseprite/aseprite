// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef UI_DRAW_H_INCLUDED
#define UI_DRAW_H_INCLUDED

#include "gfx/rect.h"
#include "gfx/region.h"
#include "ui/base.h"
#include "ui/color.h"

struct FONT;
struct BITMAP;

// TODO all these functions are deprecated and must be replaced by Graphics methods.

namespace ui {

  void jrectedge(struct BITMAP *bmp, int x1, int y1, int x2, int y2,
                 ui::Color c1, ui::Color c2);
  void jrectexclude(struct BITMAP *bmp, int x1, int y1, int x2, int y2,
                    int ex1, int ey1, int ex2, int ey2, ui::Color color);

  void jdraw_rect(const JRect rect, ui::Color color);
  void jdraw_rectfill(const JRect rect, ui::Color color);
  void jdraw_rectfill(const gfx::Rect& rect, ui::Color color);
  void jdraw_rectedge(const JRect rect, ui::Color c1, ui::Color c2);
  void jdraw_rectexclude(const JRect rc, const JRect exclude, ui::Color color);

  void jdraw_char(FONT* f, int chr, int x, int y, ui::Color fg, ui::Color bg, bool fill_bg);
  void jdraw_text(BITMAP* bmp, FONT* f, const char* text, int x, int y, ui::Color fg, ui::Color bg, bool fill_bg, int underline_height = 1);

  void jdraw_inverted_sprite(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);

  void ji_move_region(const gfx::Region& region, int dx, int dy);

} // namespace ui

#endif
