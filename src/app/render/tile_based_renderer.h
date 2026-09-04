// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_RENDER_TILE_BASED_RENDERER_H_INCLUDED
#define APP_RENDER_TILE_BASED_RENDERER_H_INCLUDED
#pragma once

#include "app/render/render_tile.h"
#include "app/render/renderer.h"

namespace app {

class TileBasedRenderer : public Renderer {
public:
  TileBasedRenderer(std::unique_ptr<Renderer>&& tilesRenderer);
  ~TileBasedRenderer();

  Renderer* tilesRenderer() { return m_tilesRenderer.get(); }

  const Properties& properties() const override;
  const render::BgOptions& bgOptions() const override;
  const render::Projection& projection() const override;

  void setRefLayersVisiblity(bool visible) override;
  void setNonactiveLayersOpacity(int opacity) override;
  void setNewBlendMethod(bool newBlend) override;
  void setComposeGroups(bool composeGroups) override;
  void setBgOptions(const render::BgOptions& bg) override;
  void setProjection(const render::Projection& projection) override;
  void setSampling(const os::Sampling& sampling) override;

  void setSelectedLayer(const doc::Layer* layer) override;
  void setPreviewImage(const doc::Layer* layer,
                       const doc::frame_t frame,
                       const doc::Image* image,
                       const doc::Tileset* tileset,
                       const gfx::Point& pos,
                       const doc::BlendMode blendMode) override;
  void removePreviewImage() override;
  void setExtraImage(render::ExtraType type,
                     const doc::Cel* cel,
                     const doc::Image* image,
                     const doc::BlendMode blendMode,
                     const doc::Layer* currentLayer,
                     const doc::frame_t currentFrame) override;
  void removeExtraImage() override;
  void setExtraCelInfoMap(const render::ExtraCelInfoMap* map) override;
  void removeExtraCelInfoMap() override;
  void setOnionskin(const render::OnionskinOptions& options) override;
  void disableOnionskin() override;

  void renderCanvas(Editor* editor,
                    ui::Graphics* g,
                    const doc::Sprite* sprite,
                    doc::frame_t frame,
                    const gfx::Rect& dest,
                    const gfx::Rect& expose,
                    bool exposeWithProj) override;

  void renderSpriteArea(os::Surface* dstSurface,
                        const doc::Sprite* sprite,
                        const doc::frame_t frame,
                        const gfx::ClipF& area) override;
  void renderCheckeredBackground(os::Surface* dstSurface,
                                 const doc::Sprite* sprite,
                                 const gfx::Clip& area) override;

  // Used to invalidate cached tiles.
  void invalidateRenderCache(const doc::Sprite* sprite) override;
  void invalidateRenderCache(const doc::Sprite* sprite, const gfx::Region& spriteRegion) override;

private:
  // Sub-renderer used to paint tiles.
  std::unique_ptr<Renderer> m_tilesRenderer;

  // Cached parts already rendered for specific sprites.
  RenderTileCache m_renderTileCache;
};

} // namespace app

#endif // APP_RENDER_TILE_BASED_RENDERER_H_INCLUDED
