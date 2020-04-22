// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_TOOLS_POINTER_H_INCLUDED
#define APP_TOOLS_POINTER_H_INCLUDED
#pragma once

#include "gfx/point.h"
#include "ui/pointer_type.h"

namespace app {
namespace tools {

// Simple container of mouse events information.
class Pointer {
public:
  enum Button { None, Left, Middle, Right };
  typedef ui::PointerType Type;

  Pointer()
    : m_point(0, 0)
    , m_button(None)
    , m_type(Type::Unknown)
    , m_pressure(0.0f) { }

  Pointer(const gfx::Point& point,
          const Button button,
          const Type type,
          const float pressure)
    : m_point(point)
    , m_button(button)
    , m_type(type)
    , m_pressure(pressure) { }

  const gfx::Point& point() const { return m_point; }
  Button button() const { return m_button; }
  Type type() const { return m_type; }
  float pressure() const { return m_pressure; }

private:
  gfx::Point m_point;
  Button m_button;
  Type m_type;
  float m_pressure;
};

} // namespace tools
} // namespace app

#endif
