// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
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

  // Alignment.
#define JI_HORIZONTAL   0x0001
#define JI_VERTICAL     0x0002
#define JI_LEFT         0x0004
#define JI_CENTER       0x0008
#define JI_RIGHT        0x0010
#define JI_TOP          0x0020
#define JI_MIDDLE       0x0040
#define JI_BOTTOM       0x0080
#define JI_HOMOGENEOUS  0x0100
#define JI_WORDWRAP     0x0200

  // Widget flags.
#define JI_HIDDEN       0x0001 // Is hidden (not visible, not clickeable).
#define JI_SELECTED     0x0002 // Is selected.
#define JI_DISABLED     0x0004 // Is disabled (not usable).
#define JI_HASFOCUS     0x0008 // Has the input focus.
#define JI_HASMOUSE     0x0010 // Has the mouse.
#define JI_HASCAPTURE   0x0020 // Captured the mouse .
#define JI_FOCUSSTOP    0x0040 // The widget support the focus on it.
#define JI_FOCUSMAGNET  0x0080 // The widget wants the focus by default (e.g. when the dialog is shown by first time).
#define JI_EXPANSIVE    0x0100 // Is expansive (want more space).
#define JI_DECORATIVE   0x0200 // To decorate windows.
#define JI_INITIALIZED  0x0400 // The widget was already initialized by a theme.
#define JI_DIRTY        0x0800 // The widget (or one child) is dirty (update_region != empty).
#define JI_HASTEXT      0x1000 // The widget has text (at least setText() was called one time).
#define JI_DOUBLECLICKABLE 0x2000 // The widget accepts double-clicks

  class GuiSystem {
  public:
    GuiSystem();
    ~GuiSystem();
  };

} // namespace ui

#endif  // UI_BASE_H_INCLUDED
