// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_SYSTEM_H_INCLUDED
#define UI_SYSTEM_H_INCLUDED
#pragma once

#include "gfx/fwd.h"
#include "ui/base.h"
#include "ui/cursor_type.h"
#include "ui/mouse_buttons.h"

namespace she { class Display; }

namespace ui {

  class Widget;

  class UISystem {
  public:
    UISystem();
    ~UISystem();
  };

  int display_w();
  int display_h();

  // Timer related

  int clock();

  // Mouse related

  // Updates the position of the mouse cursor overlay depending on the
  // current mouse position.
  void update_cursor_overlay();

  void set_use_native_cursors(bool state);
  CursorType get_mouse_cursor();
  void set_mouse_cursor(CursorType type);

  void hide_mouse_cursor();
  void show_mouse_cursor();

  void _internal_set_mouse_display(she::Display* display);
  void _internal_no_mouse_position();
  void _internal_set_mouse_position(const gfx::Point& newPos);
  void _internal_set_mouse_buttons(MouseButtons buttons);
  MouseButtons _internal_get_mouse_buttons();

  const gfx::Point& get_mouse_position();
  void set_mouse_position(const gfx::Point& newPos);

} // namespace ui

#endif
