// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

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
    bool onProcessMessage(Message* msg) OVERRIDE;
    void onPreferredSize(PreferredSizeEvent& ev) OVERRIDE;
    void onPaint(PaintEvent& ev) OVERRIDE;

  private:
    void set_position(JRect rect);
  };

} // namespace ui

#endif
