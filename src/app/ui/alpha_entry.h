// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_ALPHA_ENTRY_H_INCLUDED
#define APP_UI_ALPHA_ENTRY_H_INCLUDED
#pragma once

#include "app/ui/alpha_slider.h"
#include "ui/int_entry.h"

#include <memory>

using namespace ui;

namespace ui {
class CloseEvent;
class PopupWindow;
} // namespace ui

namespace app {

class AlphaEntry : public IntEntry {
public:
  AlphaEntry(AlphaSlider::Type type);

  int getValue() const override;
  void setValue(int value) override;
};

} // namespace app

#endif
