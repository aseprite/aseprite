// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/draw.h"

#include "ui/intern.h"
#include "ui/system.h"
#include "ui/widget.h"

#include <allegro.h>
#include <vector>

namespace ui {

using namespace gfx;

void _move_region(const Region& region, int dx, int dy)
{
  size_t nrects = region.size();

  // Blit directly screen to screen.
  if (is_linear_bitmap(ji_screen) && nrects == 1) {
    Rect rc = region[0];
    blit(ji_screen, ji_screen, rc.x, rc.y, rc.x+dx, rc.y+dy, rc.w, rc.h);
  }
  // Blit saving areas and copy them.
  else if (nrects > 1) {
    std::vector<BITMAP*> images(nrects);
    Region::const_iterator it, begin=region.begin(), end=region.end();
    BITMAP* bmp;
    int c;

    for (c=0, it=begin; it != end; ++it, ++c) {
      const Rect& rc = *it;
      bmp = create_bitmap(rc.w, rc.h);
      blit(ji_screen, bmp, rc.x, rc.y, 0, 0, bmp->w, bmp->h);
      images[c] = bmp;
    }

    for (c=0, it=begin; it != end; ++it, ++c) {
      const Rect& rc = *it;
      bmp = images[c];
      blit(bmp, ji_screen, 0, 0, rc.x+dx, rc.y+dy, bmp->w, bmp->h);
      destroy_bitmap(bmp);
    }
  }
}

} // namespace ui
