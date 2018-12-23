// Aseprite UI Library
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/manager.h"

#include "os/display.h"
#include "os/surface.h"
#include "os/system.h"
#include "ui/overlay_manager.h"

#include <vector>

namespace ui {

using namespace gfx;

void move_region(Manager* manager, const Region& region, int dx, int dy)
{
  os::Display* display = manager->getDisplay();
  ASSERT(display);
  if (!display)
    return;

  auto overlays = ui::OverlayManager::instance();
  gfx::Rect bounds = region.bounds();
  bounds |= gfx::Rect(bounds).offset(dx, dy);
  overlays->restoreOverlappedAreas(bounds);

  os::Surface* surface = display->getSurface();
  os::SurfaceLock lock(surface);
  std::size_t nrects = region.size();

  // Fast path, move one rectangle.
  if (nrects == 1) {
    gfx::Rect rc = region[0];
    surface->scrollTo(rc, dx, dy);

    rc.offset(dx, dy);
    manager->dirtyRect(rc);
  }
  // As rectangles in the region internals are separated by bands
  // through the y-axis, we can sort the rectangles by y-axis and then
  // by x-axis to move rectangle by rectangle depending on the dx/dy
  // direction so we don't overlap each rectangle.
  else if (nrects > 1) {
    std::vector<gfx::Rect> rcs(nrects);
    std::copy(region.begin(), region.end(), rcs.begin());

    std::sort(
      rcs.begin(), rcs.end(),
      [dx, dy](const gfx::Rect& a, const gfx::Rect& b){
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
      manager->dirtyRect(rc);
    }
  }

  overlays->drawOverlays();
}

} // namespace ui
