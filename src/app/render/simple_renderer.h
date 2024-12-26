// Aseprite
// Copyright (C) 2022-2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_RENDER_SIMPLE_RENDERER_H_INCLUDED
#define APP_RENDER_SIMPLE_RENDERER_H_INCLUDED
#pragma once

#include "app/render/renderer.h"

namespace app {

// Represents the way to render sprites on Aseprite (Old and "New")
// which use the render::Render class to render sprites with the
// CPU-only.
class SimpleRenderer : public Renderer {
public:
  SimpleRenderer();

  const Properties& properties() const override { return m_properties; }

  void setRefLayersVisiblity(const bool visible) override;
  void setNonactiveLayersOpacity(const int opacity) override;
  void setNewBlendMethod(const bool newBlend) override;
  void setBgOptions(const render::BgOptions& bg) override;
  void setProjection(const render::Projection& projection) override;

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
  void setOnionskin(const render::OnionskinOptions& options) override;
  void disableOnionskin() override;

  void renderSprite(os::Surface* dstSurface,
                    const doc::Sprite* sprite,
                    const doc::frame_t frame,
                    const gfx::ClipF& area) override;
  void renderCheckeredBackground(os::Surface* dstSurface,
                                 const doc::Sprite* sprite,
                                 const gfx::Clip& area) override;
  void renderImage(doc::Image* dstImage,
                   const doc::Image* srcImage,
                   const doc::Palette* pal,
                   const int x,
                   const int y,
                   const int opacity,
                   const doc::BlendMode blendMode) override;

private:
  Properties m_properties;
  render::Render m_render;
};

} // namespace app

#endif
