// Aseprite UI Library
// Copyright (C) 2018-2024  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "ui/layer.h"

#include "os/surface.h"

namespace ui {

void UILayer::reset()
{
  m_surface.reset();
}

void UILayer::setSurface(const os::SurfaceRef& newSurface)
{
  m_surface = newSurface;
}

void UILayer::setPosition(const gfx::Point& pos)
{
  m_pos = pos;
}

gfx::Rect UILayer::bounds() const
{
  if (!m_surface)
    return gfx::Rect();

  return gfx::Rect(m_pos.x, m_pos.y, m_surface->width(), m_surface->height());
}

} // namespace ui
