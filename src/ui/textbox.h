// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef UI_TEXTBOX_H_INCLUDED
#define UI_TEXTBOX_H_INCLUDED

#include "base/compiler_specific.h"
#include "ui/widget.h"

namespace ui {

  class TextBox : public Widget
  {
  public:
    TextBox(const char* text, int align);

  protected:
    bool onProcessMessage(Message* msg) OVERRIDE;
    void onPaint(PaintEvent& ev) OVERRIDE;
    void onPreferredSize(PreferredSizeEvent& ev) OVERRIDE;
    void onSetText() OVERRIDE;
  };

} // namespace ui

#endif
