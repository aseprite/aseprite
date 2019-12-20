// Aseprite UI Library
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/clamp.h"
#include "gfx/rect.h"
#include "ui/base.h"
#include "ui/system.h"

namespace ui {

int fit_bounds(int arrowAlign, const gfx::Rect& target, gfx::Rect& bounds)
{
  bounds.x = target.x;
  bounds.y = target.y;

  int trycount = 0;
  for (; trycount < 4; ++trycount) {
    switch (arrowAlign) {
      case TOP | LEFT:
        bounds.x = target.x + target.w;
        bounds.y = target.y + target.h;
        break;
      case TOP | RIGHT:
        bounds.x = target.x - bounds.w;
        bounds.y = target.y + target.h;
        break;
      case BOTTOM | LEFT:
        bounds.x = target.x + target.w;
        bounds.y = target.y - bounds.h;
        break;
      case BOTTOM | RIGHT:
        bounds.x = target.x - bounds.w;
        bounds.y = target.y - bounds.h;
        break;
      case TOP:
        bounds.x = target.x + target.w/2 - bounds.w/2;
        bounds.y = target.y + target.h;
        break;
      case BOTTOM:
        bounds.x = target.x + target.w/2 - bounds.w/2;
        bounds.y = target.y - bounds.h;
        break;
      case LEFT:
        bounds.x = target.x + target.w;
        bounds.y = target.y + target.h/2 - bounds.h/2;
        break;
      case RIGHT:
        bounds.x = target.x - bounds.w;
        bounds.y = target.y + target.h/2 - bounds.h/2;
        break;
    }

    bounds.x = base::clamp(bounds.x, 0, ui::display_w()-bounds.w);
    bounds.y = base::clamp(bounds.y, 0, ui::display_h()-bounds.h);

    if (target.intersects(bounds)) {
      switch (trycount) {
        case 0:
        case 2:
          // Switch position
          if (arrowAlign & (TOP | BOTTOM)) arrowAlign ^= TOP | BOTTOM;
          if (arrowAlign & (LEFT | RIGHT)) arrowAlign ^= LEFT | RIGHT;
          break;
        case 1:
          // Rotate positions
          if (arrowAlign & (TOP | LEFT)) arrowAlign ^= TOP | LEFT;
          if (arrowAlign & (BOTTOM | RIGHT)) arrowAlign ^= BOTTOM | RIGHT;
          break;
      }
    }
    else
      break;
  }

  return arrowAlign;
}

} // namespace ui
