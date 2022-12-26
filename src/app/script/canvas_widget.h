// Aseprite
// Copyright (c) 2022  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SCRIPT_CANVAS_H_INCLUDED
#define APP_SCRIPT_CANVAS_H_INCLUDED
#pragma once

#include "os/surface.h"
#include "ui/cursor_type.h"
#include "ui/widget.h"

namespace app {
namespace script {

class GraphicsContext;

// The canvas widget of a Dialog() created with Dialog:canvas{ ... }
// This is a generic widget where all its events can be listened.
class Canvas : public ui::Widget {
public:
  static ui::WidgetType Type();

  Canvas();

  void callPaint();

  void setMouseCursor(const ui::CursorType cursor) {
    m_cursorType = cursor;
  }

  obs::signal<void(GraphicsContext&)> Paint;
  obs::signal<void(ui::MouseMessage*)> MouseMove;
  obs::signal<void(ui::MouseMessage*)> MouseDown;
  obs::signal<void(ui::MouseMessage*)> MouseUp;

private:
  void onInitTheme(ui::InitThemeEvent& ev) override;
  bool onProcessMessage(ui::Message* msg) override;
  void onResize(ui::ResizeEvent& ev) override;
  void onPaint(ui::PaintEvent& ev) override;

  os::SurfaceRef m_surface;
  ui::CursorType m_cursorType = ui::kArrowCursor;
};

} // namespace script
} // namespace app

#endif
