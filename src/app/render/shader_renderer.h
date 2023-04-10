// Aseprite
// Copyright (C) 2022-2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_RENDER_SHADER_RENDERER_H_INCLUDED
#define APP_RENDER_SHADER_RENDERER_H_INCLUDED
#pragma once

#if SK_ENABLE_SKSL

#include "app/render/renderer.h"
#include "doc/palette.h"

#include "include/core/SkRefCnt.h"

class SkCanvas;
class SkRuntimeEffect;

namespace doc {
  class RenderPlan;
}

namespace app {

  // Use SkSL to compose images with Skia shaders on the CPU (with the
  // SkSL VM) or GPU-accelerated (with native OpenGL/Metal/etc. shaders).
  //
  // TODO This is an ongoing effort, not yet ready for production, and
  //      only accessible when ENABLE_DEVMODE is defined.
  class ShaderRenderer : public Renderer {
  public:
    ShaderRenderer();
    ~ShaderRenderer();

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
    void renderPlan(SkCanvas* canvas,
                    const doc::Sprite* sprite,
                    const doc::RenderPlan& plan,
                    const doc::frame_t frame,
                    const gfx::ClipF& area);
    void drawImage(SkCanvas* canvas,
                   const doc::Image* srcImage,
                   const int x,
                   const int y,
                   const int opacity,
                   const doc::BlendMode blendMode);

    bool checkIfWeShouldUsePreview(const doc::Cel* cel) const;
    void afterBackgroundLayerIsPainted();

    Properties m_properties;
    render::BgOptions m_bgOptions;
    render::Projection m_proj;
    sk_sp<SkRuntimeEffect> m_bgEffect;
    sk_sp<SkRuntimeEffect> m_indexedEffect;
    sk_sp<SkRuntimeEffect> m_grayscaleEffect;
    const doc::Sprite* m_sprite = nullptr;
    const doc::LayerImage* m_bgLayer = nullptr;
    // TODO these members are the same as in render::Render, we should
    //      see a way to merge both
    const doc::Layer* m_selectedLayerForOpacity = nullptr;
    const doc::Layer* m_selectedLayer = nullptr;
    doc::frame_t m_selectedFrame = -1;
    const doc::Image* m_previewImage = nullptr;
    const doc::Tileset* m_previewTileset = nullptr;
    gfx::Point m_previewPos;
    doc::BlendMode m_previewBlendMode = doc::BlendMode::NORMAL;

    // Palette of 256 colors (useful for the indexed shader to set all
    // colors outside the valid range as transparent RGBA=0 values)
    doc::Palette m_palette;
  };

} // namespace app

#endif // SK_ENABLE_SKSL

#endif
