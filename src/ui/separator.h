// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_SEPARATOR_H_INCLUDED
#define UI_SEPARATOR_H_INCLUDED
#pragma once

#include "base/compiler_specific.h"
#include "ui/widget.h"

namespace ui {

  class Separator : public Widget
  {
  public:
    Separator(const std::string& text, int align);

  protected:
    void onPaint(PaintEvent& ev) OVERRIDE;
    void onPreferredSize(PreferredSizeEvent& ev) OVERRIDE;
  };

} // namespace ui

#endif
