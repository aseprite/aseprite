// SHE library
// Copyright (C) 2016-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/skia/skia_surface.h"

#include "base/file_handle.h"

#include "SkCodec.h"
#include "SkPixelRef.h"
#include "SkStream.h"

#include <memory>

namespace she {

sk_sp<SkColorSpace> SkiaSurface::m_colorSpace;

// static
Surface* SkiaSurface::loadSurface(const char* filename)
{
  FILE* f = base::open_file_raw(filename, "rb");
  if (!f)
    return nullptr;

  std::unique_ptr<SkCodec> codec(
    SkCodec::MakeFromStream(
      std::unique_ptr<SkFILEStream>(new SkFILEStream(f))));
  if (!codec)
    return nullptr;

  SkImageInfo info = codec->getInfo()
    .makeColorType(kN32_SkColorType)
    .makeAlphaType(kPremul_SkAlphaType)
    .makeColorSpace(colorSpace());
  SkBitmap bm;
  if (!bm.tryAllocPixels(info))
    return nullptr;

  const SkCodec::Result r = codec->getPixels(info, bm.getPixels(), bm.rowBytes());
  if (r != SkCodec::kSuccess)
    return nullptr;

  SkiaSurface* sur = new SkiaSurface();
  sur->swapBitmap(bm);
  return sur;
}

// static
sk_sp<SkColorSpace> SkiaSurface::colorSpace()
{
#if 0 // TODO Add support to different color spaces
  if (!m_colorSpace)
    m_colorSpace = SkColorSpace::NewNamed(SkColorSpace::kSRGB_Named);
#endif
  return m_colorSpace;
}

} // namespace she
