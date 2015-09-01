// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_INTERN_H_INCLUDED
#define UI_INTERN_H_INCLUDED
#pragma once

#include "gfx/color.h"
#include "ui/base.h"

namespace she {
  class Font;
}

namespace ui {

  class Graphics;
  class Widget;
  class Window;

  // intern.cpp

  namespace details {

    void initWidgets();
    void exitWidgets();

    void addWidget(Widget* widget);
    void removeWidget(Widget* widget);

    void resetFontAllWidgets();
    void reinitThemeForAllWidgets();

  } // namespace details

  // theme.cpp

  void drawTextBox(Graphics* g, Widget* textbox,
                   int* w, int* h, gfx::Color bg, gfx::Color fg);

} // namespace ui

#endif
