// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/copy_region.h"

#include "app/util/buffer_region.h"
#include "doc/image.h"

namespace app {
namespace cmd {

CopyRegion::CopyRegion(Image* dst, const Image* src,
                       const gfx::Region& region,
                       const gfx::Point& dstPos,
                       bool alreadyCopied)
  : WithImage(dst)
  , m_alreadyCopied(alreadyCopied)
{
  ASSERT(!region.isEmpty());

  // Create region to save/swap later
  for (const auto& rc : region) {
    gfx::Clip clip(
      rc.x+dstPos.x, rc.y+dstPos.y,
      rc.x, rc.y, rc.w, rc.h);
    if (!clip.clip(
          dst->width(), dst->height(),
          src->width(), src->height()))
      continue;

    m_region.createUnion(m_region, gfx::Region(clip.dstBounds()));
  }

  save_image_region_in_buffer(m_region, src, dstPos, m_buffer);
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
  ASSERT(image);

  swap_image_region_with_buffer(m_region, image, m_buffer);
  image->incrementVersion();
}

} // namespace cmd
} // namespace app
