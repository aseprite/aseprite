// Aseprite UI Library
// Copyright (C) 2018-2020  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_BASE_H_INCLUDED
#define UI_BASE_H_INCLUDED
#pragma once

namespace ui {

  // Widget flags
  enum {
    HIDDEN           = 0x00000001, // Is hidden (not visible, not clickeable).
    SELECTED         = 0x00000002, // Is selected.
    DISABLED         = 0x00000004, // Is disabled (not usable).
    HAS_FOCUS        = 0x00000008, // Has the input focus.
    HAS_MOUSE        = 0x00000010, // Has the mouse.
    HAS_CAPTURE      = 0x00000020, // Captured the mouse .
    FOCUS_STOP       = 0x00000040, // The widget support the focus on it.
    FOCUS_MAGNET     = 0x00000080, // The widget wants the focus by default (e.g. when the dialog is shown by first time).
    EXPANSIVE        = 0x00000100, // Is expansive (want more space).
    DECORATIVE       = 0x00000200, // To decorate windows.
    INITIALIZED      = 0x00000400, // The widget was already initialized by a theme.
    DIRTY            = 0x00000800, // The widget (or one child) is dirty (update_region != empty).
    HAS_TEXT         = 0x00001000, // The widget has text (at least setText() was called one time).
    DOUBLE_BUFFERED  = 0x00002000, // The widget is painted in a back-buffer and then flipped to the main display
    TRANSPARENT      = 0x00004000, // The widget has transparent parts that needs the background painted before
    CTRL_RIGHT_CLICK = 0x00008000, // The widget should transform Ctrl+click to right-click on OS X.
    IGNORE_MOUSE     = 0x80000000, // Don't process mouse messages for this widget (useful for labels, boxes, grids, etc.)
    PROPERTIES_MASK  = 0x8000ffff,

    HORIZONTAL       = 0x00010000,
    VERTICAL         = 0x00020000,
    LEFT             = 0x00040000,
    CENTER           = 0x00080000,
    RIGHT            = 0x00100000,
    TOP              = 0x00200000,
    MIDDLE           = 0x00400000,
    BOTTOM           = 0x00800000,
    HOMOGENEOUS      = 0x01000000,
    WORDWRAP         = 0x02000000,
    CHARWRAP         = 0x04000000,
    ALIGN_MASK       = 0x7fff0000,
  };

} // namespace ui

#endif  // UI_BASE_H_INCLUDED
