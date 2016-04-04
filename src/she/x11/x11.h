// SHE library
// Copyright (C) 2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_X11_X11_INCLUDED
#define SHE_X11_X11_INCLUDED
#pragma once

#include <X11/Xlib.h>

namespace she {

class X11 {
  static X11* m_instance;
public:
  static X11* instance();

  X11() {
    ASSERT(m_instance == nullptr);
    m_instance = this;
    m_display = XOpenDisplay(nullptr);
  }

  ~X11() {
    ASSERT(m_instance == this);
    XCloseDisplay(m_display);
    m_instance = nullptr;
  }

  ::Display* display() const { return m_display; }

private:
  ::Display* m_display;
};

} // namespace she

#endif
