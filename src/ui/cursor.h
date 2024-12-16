// Aseprite UI Library
// Copyright (C) 2020-2021  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_CURSOR_H_INCLUDED
#define UI_CURSOR_H_INCLUDED
#pragma once

#include "gfx/point.h"
#include "os/cursor.h"
#include "os/surface.h"

namespace ui {

class Cursor {
public:
  Cursor(const os::SurfaceRef& surface = nullptr, const gfx::Point& focus = gfx::Point(0, 0));

  const os::SurfaceRef& surface() const { return m_surface; }
  const gfx::Point& focus() const { return m_focus; }
  os::CursorRef nativeCursor(const int scale) const;

  void reset();

private:
  os::SurfaceRef m_surface;
  gfx::Point m_focus;
  mutable os::CursorRef m_cursor;
  mutable int m_scale;
};

} // namespace ui

#endif
