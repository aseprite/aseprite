// LAF OS Library
// Copyright (C) 2012-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_SKIA_SKIA_WINDOW_OSX_INCLUDED
#define OS_SKIA_SKIA_WINDOW_OSX_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "os/native_cursor.h"

#include <string>

namespace os {

class EventQueue;
class SkiaDisplay;
class Surface;

class SkiaWindow {
public:
  enum class Backend { NONE, GL };

  SkiaWindow(EventQueue* queue, SkiaDisplay* display,
             int width, int height, int scale);
  ~SkiaWindow();

  int scale() const;
  void setScale(int scale);
  void setVisible(bool visible);
  void maximize();
  bool isMaximized() const;
  bool isMinimized() const;
  gfx::Size clientSize() const;
  gfx::Size restoredSize() const;
  void setTitle(const std::string& title);
  void captureMouse();
  void releaseMouse();
  void setMousePosition(const gfx::Point& position);
  bool setNativeMouseCursor(NativeCursor cursor);
  bool setNativeMouseCursor(const Surface* surface,
                            const gfx::Point& focus,
                            const int scale);
  void updateWindow(const gfx::Rect& bounds);
  std::string getLayout() { return ""; }
  void setLayout(const std::string& layout) { }
  void setTranslateDeadKeys(bool state);
  void* handle();

private:
  void destroyImpl();

  class Impl;
  Impl* m_impl;

  DISABLE_COPYING(SkiaWindow);
};

} // namespace os

#endif
