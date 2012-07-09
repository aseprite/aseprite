// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef UI_INTERN_H_INCLUDED
#define UI_INTERN_H_INCLUDED

#include "ui/base.h"

struct FONT;
struct BITMAP;

namespace ui {

  class Widget;
  class Window;

  //////////////////////////////////////////////////////////////////////
  // jintern.c

  void _ji_add_widget(Widget* widget);
  void _ji_remove_widget(Widget* widget);

  void _ji_set_font_of_all_widgets(FONT* f);
  void _ji_reinit_theme_in_all_widgets();

  //////////////////////////////////////////////////////////////////////
  // jwindow.c

  bool _jwindow_is_moving();

  //////////////////////////////////////////////////////////////////////
  // theme.cpp

  void _ji_theme_draw_sprite_color(BITMAP *bmp, BITMAP *sprite,
                                   int x, int y, int color);

  void _ji_theme_textbox_draw(BITMAP *bmp, Widget* textbox,
                              int *w, int *h, int bg, int fg);

  //////////////////////////////////////////////////////////////////////
  // jfontbmp.c

  struct FONT *_ji_bitmap2font(BITMAP *bmp);

} // namespace ui

#endif
