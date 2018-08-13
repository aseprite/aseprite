// SHE library
// Copyright (C) 2016-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_X11_WINDOW_INCLUDED
#define SHE_X11_WINDOW_INCLUDED
#pragma once

#include "gfx/fwd.h"
#include "gfx/size.h"
#include "she/native_cursor.h"
#include "she/surface_list.h"

#include <X11/Xatom.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <cstring>
#include <string>

namespace she {

class Event;
class Surface;

class X11Window {
public:
  X11Window(::Display* display, int width, int height, int scale);
  ~X11Window();

  int scale() const { return m_scale; }
  void setScale(const int scale);

  void setTitle(const std::string& title);
  void setIcons(const SurfaceList& icons);

  gfx::Size clientSize() const;
  gfx::Size restoredSize() const;
  void captureMouse();
  void releaseMouse();
  void setMousePosition(const gfx::Point& position);
  void updateWindow(const gfx::Rect& bounds);
  bool setNativeMouseCursor(NativeCursor cursor);
  bool setNativeMouseCursor(const she::Surface* surface,
                            const gfx::Point& focus,
                            const int scale);

  ::Display* x11display() const { return m_display; }
  ::Window handle() const { return m_window; }
  ::GC gc() const { return m_gc; }

  void setTranslateDeadKeys(bool state) {
    // TODO
  }

  void processX11Event(XEvent& event);
  static X11Window* getPointerFromHandle(Window handle);

protected:
  virtual void queueEvent(Event& event) = 0;
  virtual void paintGC(const gfx::Rect& rc) = 0;
  virtual void resizeDisplay(const gfx::Size& sz) = 0;

private:
  bool setX11Cursor(::Cursor xcursor);
  static void addWindow(X11Window* window);
  static void removeWindow(X11Window* window);

  ::Display* m_display;
  ::Window m_window;
  ::GC m_gc;
  ::Cursor m_cursor;
  ::XcursorImage* m_xcursorImage;
  ::XIC m_xic;
  int m_scale;
};

} // namespace she

#endif
