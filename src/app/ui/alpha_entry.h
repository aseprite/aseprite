// Aseprite UI Library
// Copyright (C) 2024  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef APP_UI_ALPHA_ENTRY_H_INCLUDED
#define APP_UI_ALPHA_ENTRY_H_INCLUDED
#pragma once

#include "ui/int_entry.h"
#include "app/ui/alpha_slider.h"

#include <memory>

using namespace ui;


namespace ui {
  class CloseEvent;
  class PopupWindow;
}

namespace app {

  class AlphaEntry : public IntEntry {
  public:
    AlphaEntry(AlphaSlider::Type type);

    int getValue() const override;
    void setValue(int value) override;
  };

} // namespace ui

#endif
