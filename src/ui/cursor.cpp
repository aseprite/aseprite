// Aseprite UI Library
// Copyright (C) 2020-2021  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "ui/cursor.h"

#include "base/debug.h"
#include "os/surface.h"
#include "os/system.h"

namespace ui {

Cursor::Cursor(const os::SurfaceRef& surface, const gfx::Point& focus)
  : m_surface(surface)
  , m_focus(focus)
  , m_scale(0)
{
}

void Cursor::reset()
{
  m_surface.reset();
  m_cursor.reset();
  m_focus = gfx::Point(0, 0);
  m_scale = 0;
}

os::CursorRef Cursor::nativeCursor(const int scale) const
{
  if (m_cursor && m_scale == scale)
    return m_cursor;

  m_cursor = os::instance()->makeCursor(m_surface.get(), m_focus, m_scale = scale);
  return m_cursor;
}

} // namespace ui
