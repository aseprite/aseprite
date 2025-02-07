// Aseprite UI Library
// Copyright (C) 2019-2021  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_SYSTEM_H_INCLUDED
#define UI_SYSTEM_H_INCLUDED
#pragma once

#include "gfx/fwd.h"
#include "ui/base.h"
#include "ui/cursor_type.h"

#include <functional>
#include <string>

namespace ui {

class ClipboardDelegate;
class Cursor;
class Display;
class Widget;

class UISystem {
public:
  static UISystem* instance();

  UISystem();
  ~UISystem();

  void setClipboardDelegate(ClipboardDelegate* delegate) { m_clipboardDelegate = delegate; }
  ClipboardDelegate* clipboardDelegate() { return m_clipboardDelegate; }

private:
  ClipboardDelegate* m_clipboardDelegate;
};

void set_multiple_displays(bool multi);
bool get_multiple_displays();

void set_clipboard_text(const std::string& text);
bool get_clipboard_text(std::string& text);

// Mouse related

// Updates the position of the mouse cursor overlay depending on the
// current mouse position.
void update_cursor_overlay();

void set_use_native_cursors(bool state);
CursorType get_mouse_cursor();
void set_mouse_cursor(CursorType type, const Cursor* cursor = nullptr);
void set_mouse_cursor_scale(const int newScale);
void set_mouse_cursor_reset_info();

void hide_mouse_cursor();
void show_mouse_cursor();

void _internal_set_mouse_display(Display* display);
void _internal_no_mouse_position();

// Returns desktop/screen mouse position (relative to no-display)
gfx::Point get_mouse_position();

// Sets the mouse position relative to a specific display (or
// relative to the desktop if it's nullptr)
void set_mouse_position(const gfx::Point& newPos, Display* display);

void execute_from_ui_thread(std::function<void()>&& func);
bool is_ui_thread();
#ifdef _DEBUG
void assert_ui_thread();
#else
static inline void assert_ui_thread()
{
}
#endif

} // namespace ui

#endif
