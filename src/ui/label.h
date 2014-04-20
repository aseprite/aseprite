// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_LABEL_H_INCLUDED
#define UI_LABEL_H_INCLUDED
#pragma once

#include "base/compiler_specific.h"
#include "ui/color.h"
#include "ui/widget.h"

namespace ui {

  class Label : public Widget
  {
  public:
    Label(const base::string& text);

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
