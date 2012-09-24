// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "ui/cursor.h"

#include "she/surface.h"

namespace ui {

Cursor::Cursor(she::Surface* surface, const gfx::Point& focus)
  : m_surface(surface)
  , m_focus(focus)
{
  ASSERT(m_surface != NULL);
}

Cursor::~Cursor()
{
  m_surface->dispose();
}

} // namespace ui
