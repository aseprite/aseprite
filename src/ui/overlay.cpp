// Aseprite UI Library
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/overlay.h"

#include "os/surface.h"
#include "os/system.h"
#include "ui/manager.h"

namespace ui {

Overlay::Overlay(os::Surface* overlaySurface, const gfx::Point& pos, ZOrder zorder)
  : m_surface(overlaySurface)
  , m_overlap(nullptr)
  , m_captured(nullptr)
  , m_pos(pos)
  , m_zorder(zorder)
{
}

Overlay::~Overlay()
{
  ASSERT(!m_captured);

  if (m_surface) {
    Manager* manager = Manager::getDefault();
    if (manager)
      manager->invalidateRect(gfx::Rect(m_pos.x, m_pos.y,
                                        m_surface->width(),
                                        m_surface->height()));
    m_surface->dispose();
  }

  if (m_overlap)
    m_overlap->dispose();
}

os::Surface* Overlay::setSurface(os::Surface* newSurface)
{
  os::Surface* oldSurface = m_surface;
  m_surface = newSurface;
  return oldSurface;
}

gfx::Rect Overlay::bounds() const
{
  if (m_surface)
    return gfx::Rect(m_pos.x, m_pos.y, m_surface->width(), m_surface->height());
  else
    return gfx::Rect(0, 0, 0, 0);
}

void Overlay::drawOverlay()
{
  if (!m_surface ||
      !m_captured)
    return;

  os::SurfaceLock lock(m_surface);
  m_captured->drawRgbaSurface(m_surface, m_pos.x, m_pos.y);

  Manager::getDefault()->dirtyRect(
    gfx::Rect(m_pos.x, m_pos.y,
              m_surface->width(),
              m_surface->height()));
}

void Overlay::moveOverlay(const gfx::Point& newPos)
{
  if (m_captured)
    restoreOverlappedArea(gfx::Rect());

  m_pos = newPos;
}

void Overlay::captureOverlappedArea(os::Surface* screen)
{
  if (!m_surface ||
      m_captured)
    return;

  if (!m_overlap) {
    // Use the same color space for the overlay as in the screen
    m_overlap = os::instance()->createSurface(m_surface->width(),
                                              m_surface->height(),
                                              screen->colorSpace());
  }

  os::SurfaceLock lock(m_overlap);
  screen->blitTo(m_overlap, m_pos.x, m_pos.y, 0, 0,
                 m_overlap->width(), m_overlap->height());

  m_captured = screen;
}

void Overlay::restoreOverlappedArea(const gfx::Rect& restoreBounds)
{
  if (!m_surface ||
      !m_overlap ||
      !m_captured)
    return;

  if (!restoreBounds.isEmpty() &&
      !restoreBounds.intersects(bounds()))
    return;

  os::SurfaceLock lock(m_overlap);
  m_overlap->blitTo(m_captured, 0, 0, m_pos.x, m_pos.y,
                    m_overlap->width(), m_overlap->height());

  Manager::getDefault()->dirtyRect(bounds());

  m_captured = nullptr;
}

}
