// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_COLOR_SOURCE_H_INCLUDED
#define APP_UI_COLOR_SOURCE_H_INCLUDED
#pragma once

#include "app/color.h"
#include "gfx/point.h"

namespace app {

class IColorSource {
public:
  virtual ~IColorSource() {}
  virtual app::Color getColorByPosition(const gfx::Point& pos) = 0;
};

} // namespace app

#endif
