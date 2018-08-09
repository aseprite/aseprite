// LAF OS Library
// Copyright (C) 2016-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_SKIA_SKIA_WINDOW_X11_INCLUDED
#define OS_SKIA_SKIA_WINDOW_X11_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "gfx/size.h"
#include "os/native_cursor.h"
#include "os/x11/window.h"

#include <string>

namespace os {

class EventQueue;
class SkiaDisplay;

class SkiaWindow : public X11Window {
public:
  enum class Backend { NONE, GL };

  SkiaWindow(EventQueue* queue, SkiaDisplay* display,
             int width, int height, int scale);
  ~SkiaWindow();

  void setVisible(bool visible);
  void maximize();
  bool isMaximized() const;
  bool isMinimized() const;

  std::string getLayout() { return ""; }
  void setLayout(const std::string& layout) { }

private:
  void queueEvent(Event& ev) override;
  void paintGC(const gfx::Rect& rc) override;
  void resizeDisplay(const gfx::Size& sz) override;

  EventQueue* m_queue;
  SkiaDisplay* m_display;

  DISABLE_COPYING(SkiaWindow);
};

} // namespace os

#endif
