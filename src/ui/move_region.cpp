// Aseprite UI Library
// Copyright (C) 2001-2014  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/manager.h"

#include "she/display.h"
#include "she/locked_surface.h"
#include "she/scoped_surface_lock.h"
#include "she/surface.h"
#include "she/system.h"

#include <vector>

namespace ui {

using namespace gfx;

void move_region(const Region& region, int dx, int dy)
{
  ASSERT(Manager::getDefault());
  if (!Manager::getDefault())
    return;

  she::System* system = she::instance();
  she::Display* display = Manager::getDefault()->getDisplay();
  ASSERT(display);
  if (!display)
    return;

  she::ScopedSurfaceLock lock(display->getSurface());
  size_t nrects = region.size();

  // Blit directly screen to screen.
  if (nrects == 1) {
    Rect rc = region[0];
    lock->blitTo(lock, rc.x, rc.y, rc.x+dx, rc.y+dy, rc.w, rc.h);
  }
  // Blit saving areas and copy them.
  else if (nrects > 1) {
    std::vector<she::Surface*> images(nrects);
    Region::const_iterator it, begin=region.begin(), end=region.end();
    she::Surface* sur;
    int c;

    for (c=0, it=begin; it != end; ++it, ++c) {
      const Rect& rc = *it;
      sur = system->createSurface(rc.w, rc.h);
      {
        she::ScopedSurfaceLock surlock(sur);
        lock->blitTo(surlock, rc.x, rc.y, 0, 0, rc.w, rc.h);
      }
      images[c] = sur;
    }

    for (c=0, it=begin; it != end; ++it, ++c) {
      const Rect& rc = *it;
      sur = images[c];
      {
        she::ScopedSurfaceLock surlock(sur);
        surlock->blitTo(lock, 0, 0, rc.x+dx, rc.y+dy, rc.w, rc.h);
      }
      sur->dispose();
    }
  }
}

} // namespace ui
