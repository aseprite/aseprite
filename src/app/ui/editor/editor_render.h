// Aseprite
// Copyright (C) 2019-2022  Igara Studio S.A.
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_RENDER_H_INCLUDED
#define APP_UI_EDITOR_RENDER_H_INCLUDED
#pragma once

#include "app/render/renderer.h"
#include "doc/blend_mode.h"
#include "doc/color.h"
#include "doc/frame.h"
#include "doc/image_buffer.h"
#include "doc/pixel_format.h"
#include "gfx/clip.h"
#include "gfx/point.h"
#include "render/extra_type.h"
#include "render/onionskin_options.h"
#include "render/projection.h"

namespace doc {
  class Cel;
  class Image;
  class Layer;
  class Palette;
  class Sprite;
  class Tileset;
}

namespace os {
  class Surface;
}

namespace app {
  class Doc;

  class EditorRender {
  public:
    enum Type {
      kSimpleRenderer,
      kShaderRenderer,
    };

    EditorRender();
    ~EditorRender();

    Type type() const;
    void setType(const Type type);

    const Renderer::Properties& properties() const {
      return m_renderer->properties();
    }

    void setRefLayersVisiblity(const bool visible);
    void setNonactiveLayersOpacity(const int opacity);
    void setNewBlendMethod(const bool newBlend);

    void setProjection(const render::Projection& projection);

    void setupBackground(Doc* doc, doc::PixelFormat pixelFormat);
    void setTransparentBackground();

    void setSelectedLayer(const doc::Layer* layer);

    void setPreviewImage(const doc::Layer* layer,
                         const doc::frame_t frame,
                         const doc::Image* image,
                         const doc::Tileset* tileset,
                         const gfx::Point& pos,
                         const doc::BlendMode blendMode);
    void removePreviewImage();

    void setExtraImage(
      render::ExtraType type,
      const doc::Cel* cel,
      const doc::Image* image,
      doc::BlendMode blendMode,
      const doc::Layer* currentLayer,
      doc::frame_t currentFrame);
    void removeExtraImage();

    void setOnionskin(const render::OnionskinOptions& options);
    void disableOnionskin();

    void renderSprite(
      os::Surface* dstSurface,
      const doc::Sprite* sprite,
      doc::frame_t frame,
      const gfx::ClipF& area);
    void renderCheckeredBackground(
      os::Surface* dstSurface,
      const doc::Sprite* sprite,
      const gfx::Clip& area);
    void renderImage(
      doc::Image* dst_image,
      const doc::Image* src_image,
      const doc::Palette* pal,
      const int x,
      const int y,
      const int opacity,
      const doc::BlendMode blendMode);

    static doc::ImageBufferPtr getRenderImageBuffer();

  private:
    std::unique_ptr<Renderer> m_renderer;
  };

} // namespace app

#endif
