// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/copy_region.h"

#include "doc/image.h"

#include <algorithm>

namespace app {
namespace cmd {

CopyRegion::CopyRegion(Image* dst, const Image* src,
                       const gfx::Region& region,
                       int dst_dx, int dst_dy,
                       bool alreadyCopied)
  : WithImage(dst)
  , m_size(0)
  , m_alreadyCopied(alreadyCopied)
{
  // Create region to save/swap later
  for (const auto& rc : region) {
    gfx::Clip clip(
      rc.x+dst_dx, rc.y+dst_dy,
      rc.x, rc.y, rc.w, rc.h);
    if (!clip.clip(
          dst->width(), dst->height(),
          src->width(), src->height()))
      continue;

    m_region.createUnion(m_region, gfx::Region(clip.dstBounds()));
  }

  // Save region pixels
  for (const auto& rc : m_region) {
    for (int y=0; y<rc.h; ++y) {
      m_stream.write(
        (const char*)src->getPixelAddress(rc.x, rc.y+y),
        src->getRowStrideSize(rc.w));
    }
  }
  m_size = size_t(m_stream.tellp());
}

void CopyRegion::onExecute()
{
  if (!m_alreadyCopied)
    swap();
}

void CopyRegion::onUndo()
{
  swap();
}

void CopyRegion::onRedo()
{
  swap();
}

void CopyRegion::swap()
{
  Image* image = this->image();

  // Save current image region in "tmp" stream
  std::stringstream tmp;
  for (const auto& rc : m_region)
    for (int y=0; y<rc.h; ++y)
      tmp.write(
        (const char*)image->getPixelAddress(rc.x, rc.y+y),
        image->getRowStrideSize(rc.w));

  // Restore m_stream into the image
  m_stream.seekg(0, std::ios_base::beg);
  for (const auto& rc : m_region) {
    for (int y=0; y<rc.h; ++y) {
      m_stream.read(
        (char*)image->getPixelAddress(rc.x, rc.y+y),
        image->getRowStrideSize(rc.w));
    }
  }

  // TODO use m_stream.swap(tmp) when clang and gcc support it
  m_stream.str(tmp.str());
  m_stream.clear();

  image->incrementVersion();
}

} // namespace cmd
} // namespace app
