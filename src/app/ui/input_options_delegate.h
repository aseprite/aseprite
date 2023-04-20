// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_INPUT_OPTIONS_DELEGATE_H_INCLUDED
#define APP_INPUT_OPTIONS_DELEGATE_H_INCLUDED
#pragma once

namespace ui {

  class InputOptionsDelegate {
  public:
    virtual ~InputOptionsDelegate() { }
    virtual void scrollWheelSensitivity(double sens) = 0;
    virtual double scrollWheelSensitivity() = 0;
  };

} // namespace app

#endif
