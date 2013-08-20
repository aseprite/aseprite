// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef UI_VIEWPORT_H_INCLUDED
#define UI_VIEWPORT_H_INCLUDED

#include "base/compiler_specific.h"
#include "ui/widget.h"

namespace ui {

  class Viewport : public Widget
  {
  public:
    Viewport();

    gfx::Size calculateNeededSize();

  protected:
    // Events
    void onResize(ResizeEvent& ev) OVERRIDE;
    void onPreferredSize(PreferredSizeEvent& ev) OVERRIDE;
    void onPaint(PaintEvent& ev) OVERRIDE;
  };

} // namespace ui

#endif
