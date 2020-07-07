// Aseprite UI Library
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_CURSOR_H_INCLUDED
#define UI_CURSOR_H_INCLUDED
#pragma once

#include "gfx/point.h"
#include "os/surface.h"

namespace os { class Surface; }

namespace ui {

  class Cursor {
  public:
    Cursor(const os::SurfaceRef& surface, const gfx::Point& focus);

    const os::SurfaceRef& getSurface() const { return m_surface; }
    const gfx::Point& getFocus() const { return m_focus; }

  private:
    os::SurfaceRef m_surface;
    gfx::Point m_focus;
  };

} // namespace ui

#endif
