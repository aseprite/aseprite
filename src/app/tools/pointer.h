// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_TOOLS_POINTER_H_INCLUDED
#define APP_TOOLS_POINTER_H_INCLUDED
#pragma once

#include "gfx/point.h"

namespace app {
namespace tools {

// Simple container of mouse events information.
class Pointer {
public:
  enum Button { None, Left, Middle, Right };

  Pointer()
    : m_point(0, 0), m_button(None) { }

  Pointer(const gfx::Point& point, Button button)
    : m_point(point), m_button(button) { }

  const gfx::Point& point() const { return m_point; }
  Button button() const { return m_button; }

private:
  gfx::Point m_point;
  Button m_button;
};

} // namespace tools
} // namespace app

#endif
