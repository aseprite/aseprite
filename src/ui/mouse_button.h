// Aseprite UI Library
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_MOUSE_BUTTON_H_INCLUDED
#define UI_MOUSE_BUTTON_H_INCLUDED
#pragma once

namespace ui {

  enum MouseButton {            // Matches the values of os::Event::MouseButton
    kButtonNone = 0,
    kButtonLeft = 1,
    kButtonRight = 2,
    kButtonMiddle = 3,
    kButtonX1 = 4,
    kButtonX2 = 5,
  };

} // namespace ui

#endif
