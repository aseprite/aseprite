// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_LABEL_H_INCLUDED
#define UI_LABEL_H_INCLUDED
#pragma once

#include "gfx/color.h"
#include "ui/widget.h"

namespace ui {

  class Label : public Widget
  {
  public:
    Label(const std::string& text);

    gfx::Color getTextColor() const;
    void setTextColor(gfx::Color color);

  protected:
    void onPreferredSize(PreferredSizeEvent& ev) override;
    void onPaint(PaintEvent& ev) override;

  private:
    gfx::Color m_textColor;
  };

} // namespace ui

#endif
