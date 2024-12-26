// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/copy_region.h"

#include "app/doc.h"
#include "app/util/buffer_region.h"
#include "doc/image.h"
#include "doc/sprite.h"
#include "doc/tileset.h"

namespace app { namespace cmd {

CopyRegion::CopyRegion(Image* dst,
                       const Image* src,
                       const gfx::Region& region,
                       const gfx::Point& dstPos,
                       bool alreadyCopied)
  : WithImage(dst)
  , m_alreadyCopied(alreadyCopied)
{
  ASSERT(!region.isEmpty());

  gfx::Rect rc = region.bounds();
  gfx::Clip clip(rc.x + dstPos.x, rc.y + dstPos.y, rc.x, rc.y, rc.w, rc.h);
  if (clip.clip(dst->width(), dst->height(), src->width(), src->height())) {
    // Create region to save/swap later
    m_region = region;
    m_region.offset(dstPos);
    m_region &= gfx::Region(clip.dstBounds());
  }

  save_image_region_in_buffer(m_region, src, dstPos, m_buffer);
}

CopyTileRegion::CopyTileRegion(Image* dst,
                               const Image* src,
                               const gfx::Region& region,
                               const gfx::Point& dstPos,
                               bool alreadyCopied,
                               const doc::tile_index tileIndex,
                               const doc::Tileset* tileset)
  : CopyRegion(dst, src, region, dstPos, alreadyCopied)
  , m_tileIndex(tileIndex)
  , m_tilesetId(tileset ? tileset->id() : NullId)
{
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

  rehash();
}

void CopyTileRegion::rehash()
{
  ASSERT(m_tileIndex != notile);
  ASSERT(m_tilesetId != NullId);
  if (m_tilesetId != NullId) {
    auto tileset = get<Tileset>(m_tilesetId);
    ASSERT(tileset);
    if (tileset) {
      tileset->incrementVersion();
      tileset->notifyTileContentChange(m_tileIndex);

      // Notify that the tileset changed
      static_cast<Doc*>(tileset->sprite()->document())->notifyTilesetChanged(tileset);
    }
  }
}

}} // namespace app::cmd
