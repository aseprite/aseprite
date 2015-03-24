// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_COMMON_LOCKED_SURFACE_H
#define SHE_COMMON_LOCKED_SURFACE_H
#pragma once

#include "gfx/clip.h"
#include "she/common/font.h"
#include "she/locked_surface.h"
#include "she/scoped_surface_lock.h"

namespace she {

namespace  {

#define INT_MULT(a, b, t)                               \
  ((t) = (a) * (b) + 0x80, ((((t) >> 8) + (t)) >> 8))

gfx::Color blend(const gfx::Color back, gfx::Color front)
{
  int t;

  if (gfx::geta(back) == 0) {
    return front;
  }
  else if (gfx::geta(front) == 0) {
    return back;
  }
  else {
    int B_r, B_g, B_b, B_a;
    int F_r, F_g, F_b, F_a;
    int D_r, D_g, D_b, D_a;

    B_r = gfx::getr(back);
    B_g = gfx::getg(back);
    B_b = gfx::getb(back);
    B_a = gfx::geta(back);

    F_r = gfx::getr(front);
    F_g = gfx::getg(front);
    F_b = gfx::getb(front);
    F_a = gfx::geta(front);

    D_a = B_a + F_a - INT_MULT(B_a, F_a, t);
    D_r = B_r + (F_r-B_r) * F_a / D_a;
    D_g = B_g + (F_g-B_g) * F_a / D_a;
    D_b = B_b + (F_b-B_b) * F_a / D_a;

    return gfx::rgba(D_r, D_g, D_b, D_a);
  }
}

} // anoynmous namespace

class CommonLockedSurface : public LockedSurface {
public:

  void drawColoredRgbaSurface(const LockedSurface* src, gfx::Color fg, gfx::Color bg, const gfx::Clip& clip) override {
    SurfaceFormatData format;
    src->getFormat(&format);

    ASSERT(format.format == kRgbaSurfaceFormat);
    ASSERT(format.bitsPerPixel == 32);

    for (int v=0; v<clip.size.h; ++v) {
      const uint32_t* ptr = (const uint32_t*)src->getData(
        clip.src.x, clip.src.y+v);

      for (int u=0; u<clip.size.w; ++u) {
        gfx::Color dstColor = getPixel(clip.dst.x+u, clip.dst.y+v);
        if (gfx::geta(bg) > 0)
          dstColor = blend(dstColor, bg);

        int srcAlpha = (((*ptr) & format.alphaMask) >> format.alphaShift);
        if (srcAlpha > 0)
          dstColor = blend(dstColor, gfx::seta(fg, srcAlpha));

        putPixel(dstColor, clip.dst.x+u, clip.dst.y+v);
        ++ptr;
      }
    }
  }

  void drawChar(Font* font, gfx::Color fg, gfx::Color bg, int x, int y, int chr) override {
    CommonFont* commonFont = static_cast<CommonFont*>(font);

    gfx::Rect charBounds = commonFont->getCharBounds(chr);
    if (!charBounds.isEmpty()) {
      ScopedSurfaceLock lock(commonFont->getSurfaceSheet());
      drawColoredRgbaSurface(lock, fg, bg, gfx::Clip(x, y, charBounds));
    }
  }

  void drawString(Font* font, gfx::Color fg, gfx::Color bg, int x, int y, const std::string& str) override {
    base::utf8_const_iterator it(str.begin()), end(str.end());
    while (it != end) {
      drawChar(font, fg, bg, x, y, *it);
      x += font->charWidth(*it);
      ++it;
    }
  }

};

} // namespace she

#endif
