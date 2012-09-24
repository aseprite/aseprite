// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef UI_CURSOR_H_INCLUDED
#define UI_CURSOR_H_INCLUDED

#include "gfx/point.h"

namespace she { class Surface; }

namespace ui {

  class Cursor {
  public:
    // The surface is disposed in ~Cursor.
    Cursor(she::Surface* surface, const gfx::Point& focus);
    ~Cursor();

    she::Surface* getSurface() { return m_surface; }
    const gfx::Point& getFocus() const { return m_focus; }

  private:
    she::Surface* m_surface;
    gfx::Point m_focus;
  };

} // namespace ui

#endif
