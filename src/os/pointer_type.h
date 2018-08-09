// LAF OS Library
// Copyright (C) 2016-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_POINTER_TYPE_H_INCLUDED
#define OS_POINTER_TYPE_H_INCLUDED
#pragma once

namespace os {

  // Source of a mouse like event
  enum class PointerType {
    Unknown,
    Mouse,                      // A regular mouse
    Touchpad,                   // Touchpad/trackpad
    Touch,                      // Touch screen
    Pen,                        // Stylus pen
    Cursor,                     // Puck like device
    Eraser                      // Eraser end of a stylus pen
  };

} // namespace os

#endif
