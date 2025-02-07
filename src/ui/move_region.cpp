// Aseprite UI Library
// Copyright (C) 2018-2021  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "ui/manager.h"

#include "os/surface.h"
#include "os/system.h"
#include "os/window.h"
#include "ui/overlay_manager.h"

#include <vector>

namespace ui {

using namespace gfx;

void move_region(Display* display, const Region& region, int dx, int dy)
{
  ASSERT(display);
  if (!display)
    return;

  os::Window* window = display->nativeWindow();
  ASSERT(window);
  if (!window)
    return;

  auto overlays = ui::OverlayManager::instance();
  gfx::Rect bounds = region.bounds();
  bounds |= gfx::Rect(bounds).offset(dx, dy);
  overlays->restoreOverlappedAreas(bounds);

  os::Surface* surface = window->surface();
  os::SurfaceLock lock(surface);

  // Fast path, move one rectangle.
  if (region.isRect()) {
    gfx::Rect rc = region.bounds();
    surface->scrollTo(rc, dx, dy);

    rc.offset(dx, dy);
    display->dirtyRect(rc);
  }
  // As rectangles in the region internals are separated by bands
  // through the y-axis, we can sort the rectangles by y-axis and then
  // by x-axis to move rectangle by rectangle depending on the dx/dy
  // direction so we don't overlap each rectangle.
  else if (region.isComplex()) {
    std::size_t nrects = region.size();
    std::vector<gfx::Rect> rcs(nrects);
    std::copy(region.begin(), region.end(), rcs.begin());

    std::sort(rcs.begin(), rcs.end(), [dx, dy](const gfx::Rect& a, const gfx::Rect& b) {
      if (dy < 0) {
        if (a.y < b.y)
          return true;
        else if (a.y == b.y) {
          if (dx < 0)
            return a.x < b.x;
          else
            return a.x > b.x;
        }
        else
          return false;
      }
      else {
        if (a.y > b.y)
          return true;
        else if (a.y == b.y) {
          if (dx < 0)
            return a.x < b.x;
          else
            return a.x > b.x;
        }
        else
          return false;
      }
    });

    for (gfx::Rect& rc : rcs) {
      surface->scrollTo(rc, dx, dy);

      rc.offset(dx, dy);
      display->dirtyRect(rc);
    }
  }

  overlays->drawOverlays();
}

} // namespace ui
