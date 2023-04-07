// Aseprite
// Copyright (C) 2022-2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_RENDER_RENDERER_H_INCLUDED
#define APP_RENDER_RENDERER_H_INCLUDED
#pragma once

#include "render/render.h"

namespace doc {
  class Image;
  class Sprite;
}

namespace os {
  class Surface;
}

namespace app {

  // Abstract class to render images from any editor to be displayed
  // in the screen mainly (to render images in files you can continue
  // using render::Render).
  class Renderer {
  public:
    struct Properties {
      // True if the background should be rendered first with a
      // renderCheckeredBackground() call (in other case
      // renderSprite() will render the background too). This allows
      // to draw a background directly on the screen and then the
      // sprite in a backbuffer.
      bool renderBgOnScreen = false;

      // True if the backbuffer should has alpha channel, i.e. to
      // composite the background already painted on the screen with
      // the sprite painted in the backbuffer.
      bool requiresRgbaBackbuffer = false;

      // True if renderSprite() composite an unpremultiplied RGBA
      // surface when we draw on a transparent background.
      bool outputsUnpremultiplied = false;
    };

    virtual ~Renderer() { }

    // Returns properties of the renderer (modifies how the editor
    // uses the renderer).
    virtual const Properties& properties() const = 0;

    // ----------------------------------------------------------------------
    // Basic configuration

    virtual void setRefLayersVisiblity(const bool visible) = 0;
    virtual void setNonactiveLayersOpacity(const int opacity) = 0;
    virtual void setNewBlendMethod(const bool newBlend) = 0;
    virtual void setBgOptions(const render::BgOptions& bg) = 0;
    virtual void setProjection(const render::Projection& projection) = 0;

    // ----------------------------------------------------------------------
    // Advance configuration (for preview/brushes purposes)

    virtual void setSelectedLayer(const doc::Layer* layer) = 0;
    virtual void setPreviewImage(const doc::Layer* layer,
                                 const doc::frame_t frame,
                                 const doc::Image* image,
                                 const doc::Tileset* tileset,
                                 const gfx::Point& pos,
                                 const doc::BlendMode blendMode) = 0;
    virtual void removePreviewImage() = 0;
    virtual void setExtraImage(render::ExtraType type,
                               const doc::Cel* cel,
                               const doc::Image* image,
                               const doc::BlendMode blendMode,
                               const doc::Layer* currentLayer,
                               const doc::frame_t currentFrame) = 0;
    virtual void removeExtraImage() = 0;
    virtual void setOnionskin(const render::OnionskinOptions& options) = 0;
    virtual void disableOnionskin() = 0;

    // ----------------------------------------------------------------------
    // Compositing

    virtual void renderSprite(os::Surface* dstSurface,
                              const doc::Sprite* sprite,
                              const doc::frame_t frame,
                              const gfx::ClipF& area) = 0;
    virtual void renderCheckeredBackground(os::Surface* dstSurface,
                                           const doc::Sprite* sprite,
                                           const gfx::Clip& area) = 0;
    virtual void renderImage(doc::Image* dstImage,
                             const doc::Image* srcImage,
                             const doc::Palette* pal,
                             const int x,
                             const int y,
                             const int opacity,
                             const doc::BlendMode blendMode) = 0;
  };

} // namespace app

#endif
