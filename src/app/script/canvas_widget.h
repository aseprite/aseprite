// Aseprite
// Copyright (c) 2022  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SCRIPT_CANVAS_H_INCLUDED
#define APP_SCRIPT_CANVAS_H_INCLUDED
#pragma once

#include "os/surface.h"
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

  obs::signal<void(GraphicsContext&)> Paint;

private:
  void onInitTheme(ui::InitThemeEvent& ev) override;
  void onResize(ui::ResizeEvent& ev) override;
  void onPaint(ui::PaintEvent& ev) override;

  os::SurfaceRef m_surface;
};

} // namespace script
} // namespace app

#endif
