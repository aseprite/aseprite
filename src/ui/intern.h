// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_INTERN_H_INCLUDED
#define UI_INTERN_H_INCLUDED
#pragma once

#include "ui/base.h"
#include "ui/color.h"

struct FONT;
struct BITMAP;

namespace ui {

  class Graphics;
  class Widget;
  class Window;

  // intern.cpp

  void addWidget(Widget* widget);
  void removeWidget(Widget* widget);

  void setFontOfAllWidgets(FONT* f);
  void reinitThemeForAllWidgets();

  // theme.cpp

  void drawTextBox(Graphics* g, Widget* textbox,
                   int* w, int* h, ui::Color bg, ui::Color fg);

  // fontbmp.c

  struct FONT* bitmapToFont(BITMAP* bmp);

} // namespace ui

#endif
