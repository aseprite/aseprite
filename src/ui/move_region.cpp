// Aseprite UI Library
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/manager.h"

#include "she/display.h"
#include "she/surface.h"
#include "she/system.h"

#include <vector>

namespace ui {

using namespace gfx;

void move_region(Manager* manager, const Region& region, int dx, int dy)
{
  she::System* system = she::instance();
  she::Display* display = Manager::getDefault()->getDisplay();
  ASSERT(display);
  if (!display)
    return;

  she::Surface* surface = display->getSurface();
  she::SurfaceLock lock(surface);
  std::size_t nrects = region.size();

  // Blit directly screen to screen.
  if (nrects == 1) {
    gfx::Rect rc = region[0];
    surface->scrollTo(rc, dx, dy);

    rc.offset(dx, dy);
    Manager::getDefault()->dirtyRect(rc);
  }
  // Blit saving areas and copy them.
  else if (nrects > 1) {
    std::vector<she::Surface*> images(nrects);
    Region::const_iterator it, begin=region.begin(), end=region.end();
    int c;

    for (c=0, it=begin; it != end; ++it, ++c) {
      const Rect& rc = *it;
      she::Surface* tmpSur = system->createSurface(rc.w, rc.h);
      {
        she::SurfaceLock tmpSurLock(tmpSur);
        surface->blitTo(tmpSur, rc.x, rc.y, 0, 0, rc.w, rc.h);
      }
      images[c] = tmpSur;
    }

    for (c=0, it=begin; it != end; ++it, ++c) {
      gfx::Rect rc((*it).x+dx, (*it).y+dy, (*it).w, (*it).h);
      she::Surface* tmpSur = images[c];
      {
        she::SurfaceLock tmpSurLock(tmpSur);
        tmpSur->blitTo(surface, 0, 0, rc.x, rc.y, rc.w, rc.h);
        manager->dirtyRect(rc);
      }
      tmpSur->dispose();
    }
  }
}

} // namespace ui
