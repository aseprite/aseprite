// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_BASE_H_INCLUDED
#define UI_BASE_H_INCLUDED
#pragma once

// Get the system's definition of NULL
#include <stddef.h>

#ifndef TRUE
  #define TRUE         (-1)
  #define FALSE        (0)
#endif

#undef MIN
#undef MAX
#undef MID

#define MIN(x,y)     (((x) < (y)) ? (x) : (y))
#define MAX(x,y)     (((x) > (y)) ? (x) : (y))
#define MID(x,y,z)   MAX((x), MIN((y), (z)))

#undef ABS
#define ABS(x)       (((x) >= 0) ? (x) : (-(x)))

#undef SGN
#define SGN(x)       (((x) >= 0) ? 1 : -1)

namespace ui {

  // Alignment flags
  enum {
    HORIZONTAL  = 0x0001,
    VERTICAL    = 0x0002,
    LEFT        = 0x0004,
    CENTER      = 0x0008,
    RIGHT       = 0x0010,
    TOP         = 0x0020,
    MIDDLE      = 0x0040,
    BOTTOM      = 0x0080,
    HOMOGENEOUS = 0x0100,
    WORDWRAP    = 0x0200,
  };

  // Widget flags
  enum {
    HIDDEN       = 0x0001, // Is hidden (not visible, not clickeable).
    SELECTED     = 0x0002, // Is selected.
    DISABLED     = 0x0004, // Is disabled (not usable).
    HAS_FOCUS    = 0x0008, // Has the input focus.
    HAS_MOUSE    = 0x0010, // Has the mouse.
    HAS_CAPTURE  = 0x0020, // Captured the mouse .
    FOCUS_STOP   = 0x0040, // The widget support the focus on it.
    FOCUS_MAGNET = 0x0080, // The widget wants the focus by default (e.g. when the dialog is shown by first time).
    EXPANSIVE    = 0x0100, // Is expansive (want more space).
    DECORATIVE   = 0x0200, // To decorate windows.
    INITIALIZED  = 0x0400, // The widget was already initialized by a theme.
    DIRTY        = 0x0800, // The widget (or one child) is dirty (update_region != empty).
    HAS_TEXT     = 0x1000, // The widget has text (at least setText() was called one time).
  };

  class GuiSystem {
  public:
    GuiSystem();
    ~GuiSystem();
  };

} // namespace ui

#endif  // UI_BASE_H_INCLUDED
