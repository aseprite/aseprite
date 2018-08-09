// LAF OS Library
// Copyright (C) 2016-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_X11_X11_INCLUDED
#define OS_X11_X11_INCLUDED
#pragma once

#include <X11/Xlib.h>

namespace os {

class X11 {
  static X11* m_instance;
public:
  static X11* instance();

  X11() {
    ASSERT(m_instance == nullptr);
    m_instance = this;
    m_display = XOpenDisplay(nullptr);
    m_xim = XOpenIM(m_display, nullptr, nullptr, nullptr);
  }

  ~X11() {
    ASSERT(m_instance == this);
    if (m_xim) XCloseIM(m_xim);
    XCloseDisplay(m_display);
    m_instance = nullptr;
  }

  ::Display* display() const { return m_display; }
  ::XIM xim() const { return m_xim; }

private:
  ::Display* m_display;
  ::XIM m_xim;
};

} // namespace os

#endif
