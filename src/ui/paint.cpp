// Aseprite UI Library
// Copyright (C) 2022  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "ui/paint.h"

#include "base/exception.h"

#if LAF_SKIA
  #include "include/core/SkBitmap.h"
#endif

namespace ui {

void set_checkered_paint_mode(Paint& paint, const int param, const gfx::Color a, const gfx::Color b)
{
#if LAF_SKIA
  SkPaint& skPaint = paint.skPaint();
  skPaint.setBlendMode(SkBlendMode::kSrcOver);

  SkBitmap bitmap;
  if (!bitmap.tryAllocPixels(SkImageInfo::MakeN32(8, 8, kOpaque_SkAlphaType))) {
    throw base::Exception("Cannot create temporary Skia surface");
  }

  {
    SkPMColor A = SkPreMultiplyARGB(gfx::geta(a), gfx::getr(a), gfx::getg(a), gfx::getb(a));
    SkPMColor B = SkPreMultiplyARGB(gfx::geta(b), gfx::getr(b), gfx::getg(b), gfx::getb(b));
    int offset = 7 - (param & 7);
    for (int y = 0; y < 8; y++)
      for (int x = 0; x < 8; x++)
        *bitmap.getAddr32(x, y) = (((x + y + offset) & 7) < 4 ? B : A);
  }

  sk_sp<SkShader> shader(
    bitmap.makeShader(SkTileMode::kRepeat, SkTileMode::kRepeat, SkSamplingOptions()));
  skPaint.setShader(shader);
#endif
}

} // namespace ui
