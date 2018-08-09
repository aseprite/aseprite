// LAF OS Library
// Copyright (C) 2012-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_COMMON_GENERIC_SURFACE_H
#define OS_COMMON_GENERIC_SURFACE_H
#pragma once

#include "gfx/clip.h"
#include "gfx/color.h"
#include "os/surface.h"

namespace os {

namespace {

#define MUL_UN8(a, b, t)                               \
  ((t) = (a) * (b) + 0x80, ((((t) >> 8) + (t)) >> 8))

inline gfx::Color blend(const gfx::Color backdrop, gfx::Color src)
{
  if (gfx::geta(backdrop) == 0)
    return src;
  else if (gfx::geta(src) == 0)
    return backdrop;

  int Br, Bg, Bb, Ba;
  int Sr, Sg, Sb, Sa;
  int Rr, Rg, Rb, Ra;

  Br = gfx::getr(backdrop);
  Bg = gfx::getg(backdrop);
  Bb = gfx::getb(backdrop);
  Ba = gfx::geta(backdrop);

  Sr = gfx::getr(src);
  Sg = gfx::getg(src);
  Sb = gfx::getb(src);
  Sa = gfx::geta(src);

  int t;
  Ra = Ba + Sa - MUL_UN8(Ba, Sa, t);
  Rr = Br + (Sr-Br) * Sa / Ra;
  Rg = Bg + (Sg-Bg) * Sa / Ra;
  Rb = Bb + (Sb-Bb) * Sa / Ra;

  return gfx::rgba(Rr, Rg, Rb, Ra);
}

} // anoynmous namespace

template<typename Base>
class GenericDrawColoredRgbaSurface : public Base {
public:

  void drawColoredRgbaSurface(const Surface* src, gfx::Color fg, gfx::Color bg, const gfx::Clip& clipbase) override {
    gfx::Clip clip(clipbase);
    if (!clip.clip(this->width(),
                   this->height(),
                   src->width(), src->height()))
      return;

    SurfaceFormatData format;
    src->getFormat(&format);

    ASSERT(format.format == kRgbaSurfaceFormat);
    ASSERT(format.bitsPerPixel == 32);

    for (int v=0; v<clip.size.h; ++v) {
      const uint32_t* ptr = (const uint32_t*)src->getData(
        clip.src.x, clip.src.y+v);

      for (int u=0; u<clip.size.w; ++u) {
        gfx::Color dstColor = this->getPixel(clip.dst.x+u, clip.dst.y+v);
        if (gfx::geta(bg) > 0)
          dstColor = blend(dstColor, bg);

        uint32_t src = (((*ptr) & format.alphaMask) >> format.alphaShift);
        if (src > 0) {
          src = gfx::rgba(gfx::getr(fg),
                          gfx::getg(fg),
                          gfx::getb(fg), src);
          dstColor = blend(dstColor, src);
        }

        this->putPixel(dstColor, clip.dst.x+u, clip.dst.y+v);
        ++ptr;
      }
    }
  }
};

} // namespace os

#endif
