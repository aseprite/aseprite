// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/overlay.h"

#include "she/locked_surface.h"
#include "she/scoped_surface_lock.h"
#include "she/system.h"
#include "ui/manager.h"

namespace ui {

Overlay::Overlay(she::Surface* overlaySurface, const gfx::Point& pos, ZOrder zorder)
  : m_surface(overlaySurface)
  , m_overlap(NULL)
  , m_pos(pos)
  , m_zorder(zorder)
{
}

Overlay::~Overlay()
{
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

she::Surface* Overlay::setSurface(she::Surface* newSurface)
{
  she::Surface* oldSurface = m_surface;
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

void Overlay::drawOverlay(she::LockedSurface* screen)
{
  if (!m_surface)
    return;

  she::ScopedSurfaceLock lockedSurface(m_surface);
  screen->drawRgbaSurface(lockedSurface, m_pos.x, m_pos.y);

  Manager::getDefault()->dirtyRect(
    gfx::Rect(m_pos.x, m_pos.y,
              m_surface->width(),
              m_surface->height()));
}

void Overlay::moveOverlay(const gfx::Point& newPos)
{
  m_pos = newPos;
}

void Overlay::captureOverlappedArea(she::LockedSurface* screen)
{
  if (!m_surface)
    return;

  if (!m_overlap)
    m_overlap = she::instance()->createSurface(m_surface->width(), m_surface->height());

  she::ScopedSurfaceLock lock(m_overlap);
  screen->blitTo(lock, m_pos.x, m_pos.y, 0, 0,
                 m_overlap->width(), m_overlap->height());
}

void Overlay::restoreOverlappedArea(she::LockedSurface* screen)
{
  if (!m_surface)
    return;

  if (!m_overlap)
    return;

  she::ScopedSurfaceLock lock(m_overlap);
  lock->blitTo(screen, 0, 0, m_pos.x, m_pos.y,
               m_overlap->width(), m_overlap->height());

  Manager::getDefault()->dirtyRect(
    gfx::Rect(m_pos.x, m_pos.y,
              m_overlap->width(),
              m_overlap->height()));
}

}
