// Aseprite UI Library
// Copyright (C) 2020  Igara Studio S.A.
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

namespace ui {

Cursor::Cursor(const os::SurfaceRef& surface, const gfx::Point& focus)
  : m_surface(surface)
  , m_focus(focus)
{
  ASSERT(m_surface != nullptr);
}

} // namespace ui
