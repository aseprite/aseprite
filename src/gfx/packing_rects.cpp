// Aseprite Gfx Library
// Copyright (C) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gfx/packing_rects.h"

#include "gfx/region.h"
#include "gfx/size.h"

namespace gfx {

void PackingRects::add(const Size& sz)
{
  m_rects.push_back(Rect(sz));
}

void PackingRects::add(const Rect& rc)
{
  m_rects.push_back(rc);
}

Size PackingRects::bestFit()
{
  Size size(0, 0);

  // Calculate the amount of pixels that we need, the texture cannot
  // be smaller than that.
  int neededArea = 0;
  for (const auto& rc : m_rects) {
    neededArea += rc.w * rc.h;
  }

  int w = 1;
  int h = 1;
  int z = 0;
  bool fit = false;
  while (true) {
    if (w*h >= neededArea) {
      fit = pack(Size(w, h));
      if (fit) {
        size = Size(w, h);
        break;
      }
    }

    if ((++z) & 1)
      w *= 2;
    else
      h *= 2;
  }

  return size;
}

static bool by_area(const Rect* a, const Rect* b) {
  return a->w*a->h > b->w*b->h;
}

bool PackingRects::pack(const Size& size)
{
  m_bounds = Rect(size);

  // We cannot sort m_rects because we want to 
  std::vector<Rect*> rectPtrs(m_rects.size());
  int i = 0;
  for (auto& rc : m_rects)
    rectPtrs[i++] = &rc;
  std::sort(rectPtrs.begin(), rectPtrs.end(), by_area);

  gfx::Region rgn(m_bounds);
  for (auto rcPtr : rectPtrs) {
    gfx::Rect& rc = *rcPtr;

    for (int v=0; v<=m_bounds.h-rc.h; ++v) {
      for (int u=0; u<=m_bounds.w-rc.w; ++u) {
        gfx::Rect possible(u, v, rc.w, rc.h);
        Region::Overlap overlap = rgn.contains(possible);
        if (overlap == Region::In) {
          rc = possible;
          rgn.createSubtraction(rgn, gfx::Region(rc));
          goto next_rc;
        }
      }
    }
    return false; // There is not enough room for "rc"
  next_rc:;
  }

  return true;
}

} // namespace gfx
