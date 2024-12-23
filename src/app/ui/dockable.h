// Aseprite
// Copyright (C) 2021  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_DOCKABLE_H_INCLUDED
#define APP_UI_DOCKABLE_H_INCLUDED
#pragma once

#include "ui/base.h"

namespace app {

class Dockable {
public:
  virtual ~Dockable() {}

  // LEFT = can be docked at the left side
  // TOP = can be docked at the top
  // RIGHT = can be docked at the right side
  // BOTTOM = can be docked at the bottom
  // CENTER = can be docked at the center
  // EXPANSIVE = can be resized (e.g. add a splitter when docked at sides)
  virtual int dockableAt() const
  {
    return ui::LEFT | ui::TOP | ui::RIGHT | ui::BOTTOM | ui::CENTER | ui::EXPANSIVE;
  }

  // Returns the preferred side where the dock handle to move the
  // widget should be.
  virtual int dockHandleSide() const { return ui::TOP; }
};

} // namespace app

#endif
