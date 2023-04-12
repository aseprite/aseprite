// Aseprite
// Copyright (c) 2022-2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SCRIPT_CANVAS_H_INCLUDED
#define APP_SCRIPT_CANVAS_H_INCLUDED
#pragma once

#include "os/surface.h"
#include "ui/cursor_type.h"
#include "ui/widget.h"

namespace ui {
  class TouchMessage;
}

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

  void setAutoScaling(const bool v) {
    m_autoScaling = v;
  }

  bool isAutoScaling() const {
    return m_autoScaling;
  }

  obs::signal<void(GraphicsContext&)> Paint;
  obs::signal<void(ui::KeyMessage*)> KeyDown;
  obs::signal<void(ui::KeyMessage*)> KeyUp;
  obs::signal<void(ui::MouseMessage*)> MouseMove;
  obs::signal<void(ui::MouseMessage*)> MouseDown;
  obs::signal<void(ui::MouseMessage*)> MouseUp;
  obs::signal<void(ui::MouseMessage*)> DoubleClick;
  obs::signal<void(ui::MouseMessage*)> Wheel;
  obs::signal<void(ui::TouchMessage*)> TouchMagnify;

  static void stopKeyEventPropagation();

private:
  static bool s_stopKeyEventPropagation;

  void onInitTheme(ui::InitThemeEvent& ev) override;
  bool onProcessMessage(ui::Message* msg) override;
  void onResize(ui::ResizeEvent& ev) override;
  void onPaint(ui::PaintEvent& ev) override;

  os::SurfaceRef m_surface;
  ui::CursorType m_cursorType = ui::kArrowCursor;

  // Flag used to indicate that the canvas will scale all the drawing operations
  // according to the UI scale's preferences setting. So the user doesn't have to
  // take care about the current scale when writing scripts.
  bool m_autoScaling = true;
};

} // namespace script
} // namespace app

#endif
