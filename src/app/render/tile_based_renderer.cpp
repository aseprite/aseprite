// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/render/tile_based_renderer.h"

#include "app/pref/preferences.h"
#include "app/render/shader_renderer.h"
#include "app/ui/editor/editor.h"
#include "base/thread_pool.h"
#include "fmt/format.h"
#include "ui/graphics.h"

namespace app {

TileBasedRenderer::TileBasedRenderer(std::unique_ptr<Renderer>&& tilesRenderer)
  : m_tilesRenderer(std::move(tilesRenderer))
{
}

TileBasedRenderer::~TileBasedRenderer()
{
}

const Renderer::Properties& TileBasedRenderer::properties() const
{
  return m_tilesRenderer->properties();
}

const render::BgOptions& TileBasedRenderer::bgOptions() const
{
  return m_tilesRenderer->bgOptions();
}

const render::Projection& TileBasedRenderer::projection() const
{
  return m_tilesRenderer->projection();
}

void TileBasedRenderer::setRefLayersVisiblity(bool visible)
{
  m_tilesRenderer->setRefLayersVisiblity(visible);
}

void TileBasedRenderer::setNonactiveLayersOpacity(int opacity)
{
  m_tilesRenderer->setNonactiveLayersOpacity(opacity);
}

void TileBasedRenderer::setNewBlendMethod(bool newBlend)
{
  m_tilesRenderer->setNewBlendMethod(newBlend);
}

void TileBasedRenderer::setComposeGroups(bool composeGroups)
{
  m_tilesRenderer->setComposeGroups(composeGroups);
}

void TileBasedRenderer::setBgOptions(const render::BgOptions& bg)
{
  m_tilesRenderer->setBgOptions(bg);
}

void TileBasedRenderer::setProjection(const render::Projection& projection)
{
  m_tilesRenderer->setProjection(projection);
}

void TileBasedRenderer::setSampling(const os::Sampling& sampling)
{
  m_tilesRenderer->setSampling(sampling);
}

void TileBasedRenderer::setSelectedLayer(const doc::Layer* layer)
{
  m_tilesRenderer->setSelectedLayer(layer);
}

void TileBasedRenderer::setPreviewImage(const doc::Layer* layer,
                                        const doc::frame_t frame,
                                        const doc::Image* image,
                                        const doc::Tileset* tileset,
                                        const gfx::Point& pos,
                                        const doc::BlendMode blendMode)
{
  m_tilesRenderer->setPreviewImage(layer, frame, image, tileset, pos, blendMode);
}

void TileBasedRenderer::removePreviewImage()
{
  m_tilesRenderer->removePreviewImage();
}

void TileBasedRenderer::setExtraImage(render::ExtraType type,
                                      const doc::Cel* cel,
                                      const doc::Image* image,
                                      const doc::BlendMode blendMode,
                                      const doc::Layer* currentLayer,
                                      const doc::frame_t currentFrame)
{
  m_tilesRenderer->setExtraImage(type, cel, image, blendMode, currentLayer, currentFrame);
}

void TileBasedRenderer::removeExtraImage()
{
  m_tilesRenderer->removeExtraImage();
}

void TileBasedRenderer::setExtraCelInfoMap(const render::ExtraCelInfoMap* map)
{
  m_tilesRenderer->setExtraCelInfoMap(map);
}

void TileBasedRenderer::removeExtraCelInfoMap()
{
  m_tilesRenderer->removeExtraCelInfoMap();
}

void TileBasedRenderer::setOnionskin(const render::OnionskinOptions& options)
{
  m_tilesRenderer->setOnionskin(options);
}

void TileBasedRenderer::disableOnionskin()
{
  m_tilesRenderer->disableOnionskin();
}

void TileBasedRenderer::renderCanvas(Editor* editor,
                                     ui::Graphics* g,
                                     const doc::Sprite* sprite,
                                     const doc::frame_t frame,
                                     const gfx::Rect& dest,
                                     const gfx::Rect& expose,
                                     const bool exposeWithProj)
{
  const auto& pref = Preferences::instance(); // TODO move these options to Renderer
  const render::Projection proj = projection();
  const gfx::Rect visible = (exposeWithProj ? expose : proj.apply(expose));

  // Render background (using a ShaderRenderer)
  // TODO create a new class to just render the background
  const doc::LayerImage* bgLayer = sprite->backgroundLayer();
  if (!bgLayer || !bgLayer->isVisible()) {
    auto renderBg = [proj, g, sprite, dest, visible](Renderer* r) {
      r->setProjection(proj);
      r->renderCheckeredBackground(
        g->getInternalSurface(),
        sprite,
        gfx::Clip(dest.x + g->getInternalDeltaX(), dest.y + g->getInternalDeltaY(), visible));
    };
    if (properties().renderBgOnScreen)
      renderBg(this);
    else {
#if SK_ENABLE_SKSL
      ShaderRenderer bgRenderer;
      bgRenderer.setBgOptions(bgOptions());
      renderBg(&bgRenderer);
#endif
    }
  }
  setBgOptions(render::BgOptions::MakeTransparent());

  // Get the cached tiles of this document, and create a
  // "renderTiles" to paint right now.
  CachedTiles& cachedTiles = m_renderTileCache.cachedTiles(sprite->id());
  std::vector<RenderTile> renderTiles;

  // Paint tiles
  const gfx::Size tileSize = RenderTile::kTileSize;
  const gfx::SizeF tileSizeSrc(tileSize.w, tileSize.h);
  const bool debugTiles = pref.render.debugTiles();

  {
    // TODO remove dependency with Editor widget
    gfx::Rect rc(editor->canvasSize());
    rc = editor->editorToScreen(rc);
    rc.offset(-editor->bounds().origin());

    gfx::Rect tilerc(tileSize);

    gfx::PointF firstTilePos;
    int u0 = std::floor(visible.x / tileSizeSrc.w);
    int v0 = std::floor(visible.y / tileSizeSrc.h);
    firstTilePos.x = u0 * tileSizeSrc.w;
    firstTilePos.y = v0 * tileSizeSrc.h;

    rc.x += (visible.x / tileSize.w) * tileSize.w;
    rc.y += (visible.y / tileSize.h) * tileSize.h;
    rc.w = ((std::max(rc.w, visible.w) + tileSize.w) / tileSize.w) * tileSize.w;
    rc.h = ((std::max(rc.h, visible.h) + tileSize.h) / tileSize.h) * tileSize.w;

    for (int y = 0, v = 0; y < dest.h + tileSize.h; y += tileSize.h, ++v) {
      for (int x = 0, u = 0; x < dest.w + tileSize.w; x += tileSize.w, ++u) {
        tilerc.x = rc.x + x;
        tilerc.y = rc.y + y;
        if (tilerc.intersects(dest)) {
          auto srcrc = gfx::RectF(firstTilePos.x + u * tileSizeSrc.w,
                                  firstTilePos.y + v * tileSizeSrc.h,
                                  tileSizeSrc.w,
                                  tileSizeSrc.h);

          const RenderTileId tileId = ((v0 + v) << 16) | (u0 + u);
          auto it = cachedTiles.find(tileId);
          if (it != cachedTiles.end()) {
            RenderTile tile = it->second;
            tile.dst = tilerc;
            renderTiles.push_back(tile);
          }
          else {
            RenderTile tile(tileId, srcrc, tilerc);
            renderTiles.push_back(tile);
          }
        }
      }
    }
  }

  // Assign a surface for each tile to be rendered
  size_t dirties = 0;
  {
    for (RenderTile& renderTile : renderTiles) {
      if (!renderTile.surface) {
        renderTile.dirty = true;
        renderTile.surface = m_renderTileCache.allocTileSurface();
      }
      if (renderTile.dirty)
        ++dirties;
    }
  }

  // Sampling for downscaling
  os::Sampling sampling(os::Sampling::Filter::Nearest);
  if (proj.scaleX() < 1.0) {
    switch (pref.editor.downsampling()) {
      case gen::Downsampling::NEAREST:
        sampling = os::Sampling(os::Sampling::Filter::Nearest);
        break;
      case gen::Downsampling::BILINEAR:
        sampling = os::Sampling(os::Sampling::Filter::Linear);
        break;
      case gen::Downsampling::BILINEAR_MIPMAP:
        sampling = os::Sampling(os::Sampling::Filter::Linear, os::Sampling::Mipmap::Nearest);
        break;
      case gen::Downsampling::TRILINEAR_MIPMAP:
        sampling = os::Sampling(os::Sampling::Filter::Linear, os::Sampling::Mipmap::Linear);
        break;
    }
  }

  // Render tiles on background
  if (dirties > 0) {
    m_tilesRenderer->setProjection(proj);
    m_tilesRenderer->setSampling(sampling);

    auto renderSpriteOnTile = [this, sprite, frame](RenderTile& renderTile) {
      renderTile.surface->clear();
      m_tilesRenderer->renderSprite(renderTile.surface.get(),
                                    sprite,
                                    frame,
                                    gfx::Clip(0, 0, renderTile.src));
      renderTile.dirty = false;
    };

    const bool useThreads = (dirties >= 4);
    if (useThreads) {
      static base::thread_pool pool(4);
      for (auto& renderTile : renderTiles) {
        if (renderTile.dirty)
          pool.execute([&renderTile, renderSpriteOnTile] { renderSpriteOnTile(renderTile); });
      }
      pool.wait_all();
    }
    else {
      for (RenderTile& renderTile : renderTiles) {
        if (renderTile.dirty)
          renderSpriteOnTile(renderTile);
      }
    }

    // Re-cache rendered tiles updating the cachedTiles map
    for (const RenderTile& renderTile : renderTiles)
      cachedTiles[renderTile.tileId] = renderTile;
  }

  // Paint tiles in the editor
  for (RenderTile& renderTile : renderTiles) {
    os::Paint p;
    p.srcEdges(os::Paint::SrcEdges::Fast);
    p.blendMode(os::BlendMode::SrcOver);
    g->drawSurface(renderTile.surface.get(),
                   renderTile.surface->bounds(),
                   renderTile.dst,
                   sampling,
                   &p);
    if (debugTiles) {
      // Paint tile edges
      p.style(os::Paint::Stroke);
      p.color(gfx::rgba(0, 0, 255, 64));
      g->drawRect(renderTile.dst, p);

      // Paint tile ID
      g->drawText(fmt::format("{},{}", renderTile.tileId & 0xffff, renderTile.tileId >> 16),
                  gfx::rgba(0, 0, 0, 200),
                  gfx::ColorNone,
                  renderTile.dst.origin() + gfx::Point(renderTile.dst.size()) / 2);
    }
  }
}

void TileBasedRenderer::renderSprite(os::Surface* dstSurface,
                                     const doc::Sprite* sprite,
                                     const doc::frame_t frame,
                                     const gfx::ClipF& area)
{
  m_tilesRenderer->renderSprite(dstSurface, sprite, frame, area);
}

void TileBasedRenderer::renderCheckeredBackground(os::Surface* dstSurface,
                                                  const doc::Sprite* sprite,
                                                  const gfx::Clip& area)
{
  m_tilesRenderer->renderCheckeredBackground(dstSurface, sprite, area);
}

void TileBasedRenderer::invalidateRenderCache(const doc::Sprite* sprite)
{
  m_renderTileCache.clearCachedTiles(sprite->id());
}

void TileBasedRenderer::invalidateRenderCache(const doc::Sprite* sprite,
                                              const gfx::Region& spriteRegion)
{
  const auto& proj = projection();
  CachedTiles& cachedTiles = m_renderTileCache.cachedTiles(sprite->id());
  for (auto& [tileId, cachedTile] : cachedTiles) {
    const gfx::Rect srcrc = proj.remove(cachedTile.src);
    if (spriteRegion.contains(srcrc) != gfx::Region::Overlap::Out) {
      cachedTile.dirty = true;
    }
  }
}

} // namespace app
