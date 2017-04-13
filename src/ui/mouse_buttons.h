// Aseprite UI Library
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_MOUSE_BUTTONS_H_INCLUDED
#define UI_MOUSE_BUTTONS_H_INCLUDED
#pragma once

namespace ui {

  enum MouseButtons {
    kButtonNone = 0,
    kButtonLeft = 1,
    kButtonRight = 2,
    kButtonMiddle = 4,
    kButtonX1 = 8,
    kButtonX2 = 16,
  };

} // namespace ui

#endif
