// SHE library
// Copyright (C) 2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_POINTER_TYPE_H_INCLUDED
#define SHE_POINTER_TYPE_H_INCLUDED
#pragma once

namespace she {

  // Source of a mouse like event
  enum class PointerType {
    Unknown,
    Mouse,                      // A regular mouse
    Multitouch,                 // Trackpad/multitouch surface
    Pen,                        // Stylus pen
    Cursor,                     // Puck like device
    Eraser                      // Eraser end of a stylus pen
  };

} // namespace she

#endif
