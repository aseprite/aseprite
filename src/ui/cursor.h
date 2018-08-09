// Aseprite UI Library
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_CURSOR_H_INCLUDED
#define UI_CURSOR_H_INCLUDED
#pragma once

#include "gfx/point.h"

namespace os { class Surface; }

namespace ui {

  class Cursor {
  public:
    // The surface is disposed in ~Cursor.
    Cursor(os::Surface* surface, const gfx::Point& focus);
    ~Cursor();

    os::Surface* getSurface() const { return m_surface; }
    const gfx::Point& getFocus() const { return m_focus; }

  private:
    os::Surface* m_surface;
    gfx::Point m_focus;
  };

} // namespace ui

#endif
