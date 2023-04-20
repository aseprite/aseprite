// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_INPUT_OPTIONS_H_INCLUDED
#define APP_INPUT_OPTIONS_H_INCLUDED
#pragma once

#include "app/pref/preferences.h"
#include "app/ui/input_options_delegate.h"

namespace app {

  class InputOptions : public ui::InputOptionsDelegate {
  public:
    static InputOptions* instance();

    InputOptions();

    InputOptions(Preferences &pref);

    ~InputOptions();

    // ui::InputOptionsDelegate impl
    void scrollWheelSensitivity(double sens) override;
    double scrollWheelSensitivity() override;

  private:
    double m_scrollWheelSensitivity;
  };

} // namespace app

#endif
