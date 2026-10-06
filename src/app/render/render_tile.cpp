// Aseprite
// Copyright (c) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/render/render_tile.h"

#include "os/system.h"

namespace app {

os::SurfaceRef RenderTileCache::allocTileSurface()
{
  if (m_freeTiles.empty())
    return os::System::instance()->makeRgbaSurface(RenderTile::kTileSize.w,
                                                   RenderTile::kTileSize.h);

  os::SurfaceRef surface = m_freeTiles.back();
  m_freeTiles.pop_back();
  return surface;
}

CachedTiles& RenderTileCache::cachedTiles(const CanvasView* view)
{
  auto it = m_views.find(view);
  if (it != m_views.end())
    return it->second->cachedTiles;

  auto cache = std::make_unique<Cache>();
  CachedTiles& cachedTiles = cache->cachedTiles;
  m_views.emplace(view, std::move(cache));
  return cachedTiles;
}

void RenderTileCache::clearCachedTiles(const CanvasView* view, bool viewIsDeleted)
{
  auto it = m_views.find(view);
  if (it == m_views.end())
    return;

  CachedTiles& cachedTiles = it->second->cachedTiles;
  for (auto& [tileId, cachedTile] : cachedTiles) {
    m_freeTiles.push_back(cachedTile.surface);
    cachedTile.surface = nullptr;
  }
  cachedTiles.clear();

  if (viewIsDeleted)
    m_views.erase(it);
}

} // namespace app
