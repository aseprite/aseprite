// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
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

struct BITMAP;

#define JI_SCREEN_W ui::ji_screen_w
#define JI_SCREEN_H ui::ji_screen_h

namespace she { class Display; }

namespace ui {

  class Widget;

  // Screen related

  extern struct BITMAP* ji_screen;
  extern int ji_screen_w;
  extern int ji_screen_h;

  // Simple flag to indicate that something in the screen was modified
  // so a flip to the real screen is needed.
  extern bool dirty_display_flag;

  void SetDisplay(she::Display* display);

  // Timer related

  extern int volatile ji_clock;   /* in milliseconds */

  // Mouse related

  // Updates the position of the mouse cursor overlay depending on the
  // current mouse position.
  void UpdateCursorOverlay();

  CursorType jmouse_get_cursor();
  void jmouse_set_cursor(CursorType type);

  void jmouse_hide();
  void jmouse_show();

  bool jmouse_is_hidden();
  bool jmouse_is_shown();

  bool jmouse_poll();

  gfx::Point get_mouse_position();
  void set_mouse_position(const gfx::Point& newPos);

  void jmouse_capture();
  void jmouse_release();

  MouseButtons jmouse_b(int antique);
  int jmouse_x(int antique);
  int jmouse_y(int antique);
  int jmouse_z(int antique);

  gfx::Point get_delta_outside_box(const gfx::Rect& rect, const gfx::Point& mousePoint);
  gfx::Point control_infinite_scroll(Widget* widget, const gfx::Rect& rect, const gfx::Point& mousePoint);

} // namespace ui

#endif
