// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef UI_DRAW_H_INCLUDED
#define UI_DRAW_H_INCLUDED

#include "gfx/rect.h"
#include "ui/base.h"

#define JI_COLOR_SHADE(color, r, g, b)          \
  makecol(MID(0, getr(color)+(r), 255),         \
          MID(0, getg(color)+(g), 255),         \
          MID(0, getb(color)+(b), 255))

#define JI_COLOR_INTERP(c1, c2, step, max)              \
  makecol(getr(c1)+(getr(c2)-getr(c1)) * step / max,    \
          getg(c1)+(getg(c2)-getg(c1)) * step / max,    \
          getb(c1)+(getb(c2)-getb(c1)) * step / max)

struct FONT;
struct BITMAP;

namespace ui {

  void jrectedge(struct BITMAP *bmp, int x1, int y1, int x2, int y2,
                 int c1, int c2);
  void jrectexclude(struct BITMAP *bmp, int x1, int y1, int x2, int y2,
                    int ex1, int ey1, int ex2, int ey2, int color);
  void jrectshade(struct BITMAP *bmp, int x1, int y1, int x2, int y2,
                  int c1, int c2, int align);

  void jdraw_rect(const JRect rect, int color);
  void jdraw_rectfill(const JRect rect, int color);
  void jdraw_rectfill(const gfx::Rect& rect, int color);
  void jdraw_rectedge(const JRect rect, int c1, int c2);
  void jdraw_rectshade(const JRect rect, int c1, int c2, int align);
  void jdraw_rectexclude(const JRect rc, const JRect exclude, int color);

  void jdraw_char(FONT* f, int chr, int x, int y, int fg, int bg, bool fill_bg);
  void jdraw_text(BITMAP* bmp, FONT* f, const char* text, int x, int y, int fg, int bg, bool fill_bg, int underline_height = 1);

  void jdraw_inverted_sprite(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);

  void ji_move_region(JRegion region, int dx, int dy);

} // namespace ui

#endif
