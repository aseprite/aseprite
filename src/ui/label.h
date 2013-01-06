// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef UI_LABEL_H_INCLUDED
#define UI_LABEL_H_INCLUDED

#include "base/compiler_specific.h"
#include "ui/color.h"
#include "ui/widget.h"

namespace ui {

  class Label : public Widget
  {
  public:
    Label(const char *text);

    Color getTextColor() const;
    void setTextColor(Color color);

  protected:
    void onPreferredSize(PreferredSizeEvent& ev) OVERRIDE;
    void onPaint(PaintEvent& ev) OVERRIDE;

  private:
    Color m_textColor;
  };

} // namespace ui

#endif
