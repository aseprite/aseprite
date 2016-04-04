// SHE library
// Copyright (C) 2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_X11_WINDOW_INCLUDED
#define SHE_X11_WINDOW_INCLUDED
#pragma once

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include <cstring>

namespace she {

class X11Window {
public:
  X11Window(::Display* display, int width, int height)
    : m_display(display) {
    ::Window root = XDefaultRootWindow(m_display);

    XSetWindowAttributes swa;
    swa.event_mask = (StructureNotifyMask | ExposureMask | PropertyChangeMask |
                      EnterWindowMask | LeaveWindowMask | FocusChangeMask |
                      ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
                      KeyPressMask | KeyReleaseMask);

    m_window = XCreateWindow(
      m_display, root,
      0, 0, width, height, 0,
      CopyFromParent,
      InputOutput,
      CopyFromParent,
      CWEventMask,
      &swa);

    XMapWindow(m_display, m_window);
    XSelectInput(m_display, m_window, StructureNotifyMask);

    m_gc = XCreateGC(m_display, m_window, 0, nullptr);
  }

  ~X11Window() {
    XFreeGC(m_display, m_gc);
    XDestroyWindow(m_display, m_window);
  }

  void setTitle(const std::string& title) {
    XTextProperty prop;
    prop.value = (unsigned char*)title.c_str();
    prop.encoding = XA_STRING;
    prop.format = 8;
    prop.nitems = std::strlen((char*)title.c_str());
    XSetWMName(m_display, m_window, &prop);
  }

  virtual void onExpose() = 0;

  ::Display* display() const { return m_display; }
  ::Window handle() const { return m_window; }
  ::GC gc() const { return m_gc; }

private:
  ::Display* m_display;
  ::Window m_window;
  ::GC m_gc;
};

} // namespace she

#endif
