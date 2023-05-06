// Aseprite Render Library
// Copyright (c) 2019-2023 Igara Studio S.A.
// Copyright (c) 2001-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef RENDER_RENDER_H_INCLUDED
#define RENDER_RENDER_H_INCLUDED
#pragma once

#include "doc/doc.h"
#include "doc/anidir.h"
#include "doc/blend_mode.h"
#include "doc/color.h"
#include "doc/frame.h"
#include "doc/pixel_format.h"
#include "gfx/clip.h"
#include "gfx/point.h"
#include "gfx/size.h"
#include "render/bg_options.h"
#include "render/extra_type.h"
#include "render/onionskin_options.h"
#include "render/projection.h"

namespace doc {
  class Cel;
  class Image;
  class Layer;
  class Palette;
  class RenderPlan;
  class Sprite;
  class Tileset;
}

namespace render {
  using namespace doc;

  typedef void (*CompositeImageFunc)(
    Image* dst,
    const Image* src,
    const Palette* pal,
    const gfx::ClipF& area,
    const int opacity,
    const BlendMode blendMode,
    const double sx,
    const double sy,
    const bool newBlend);

  class Render {
    enum Flags {
      ShowRefLayers = 1,
    };

  public:
    Render();

    void setRefLayersVisiblity(const bool visible);
    void setNonactiveLayersOpacity(const int opacity);
    void setNewBlend(const bool newBlend);
    void setProjection(const Projection& projection);
    void setBgOptions(const BgOptions& bg);
    void setSelectedLayer(const Layer* layer);

    // Sets the preview image. This preview image is an alternative
    // image to be used for the given layer/frame.
    void setPreviewImage(const Layer* layer,
                         const frame_t frame,
                         const Image* image,
                         const Tileset* tileset,
                         const gfx::Point& pos,
                         const BlendMode blendMode);
    void removePreviewImage();

    // Sets an extra cel/image to be drawn after the current
    // layer/frame.
    void setExtraImage(
      ExtraType type,
      const Cel* cel, const Image* image, BlendMode blendMode,
      const Layer* currentLayer,
      frame_t currentFrame);
    void removeExtraImage();

    void setOnionskin(const OnionskinOptions& options);
    void disableOnionskin();

    void renderSprite(
      Image* dstImage,
      const Sprite* sprite,
      frame_t frame);

    void renderLayer(
      Image* dstImage,
      const Layer* layer,
      frame_t frame);

    void renderLayer(
      Image* dstImage,
      const Layer* layer,
      frame_t frame,
      const gfx::Clip& area,
      BlendMode blendMode = BlendMode::UNSPECIFIED);

    // Main function used to render the sprite. Draws the given sprite
    // frame in a new image and return it. Note: zoomedRect must have
    // the zoom applied (zoomedRect = zoom.apply(spriteRect)).
    void renderSprite(
      Image* dstImage,
      const Sprite* sprite,
      frame_t frame,
      const gfx::ClipF& area);

    // Extra functions
    void renderCheckeredBackground(
      Image* image,
      const gfx::Clip& area);

    void renderImage(
      Image* dst_image,
      const Image* src_image,
      const Palette* pal,
      const int x,
      const int y,
      const int opacity,
      const BlendMode blendMode);

    void renderCel(
      Image* dst_image,
      const Cel* cel,
      const Sprite* sprite,
      const Image* cel_image,
      const Layer* cel_layer,
      const Palette* pal,
      const gfx::RectF& celBounds,
      const gfx::Clip& area,
      const int opacity,
      const BlendMode blendMode);

  private:
    void renderSpriteLayers(
      Image* dstImage,
      const gfx::ClipF& area,
      frame_t frame,
      CompositeImageFunc compositeImage);

    void renderBackground(
      Image* image,
      const Layer* bgLayer,
      const color_t bg_color,
      const gfx::ClipF& area);

    bool isSolidBackground(
      const Layer* bgLayer,
      const color_t bg_color) const;

    void renderOnionskin(
      Image* image,
      const gfx::Clip& area,
      const frame_t frame,
      const CompositeImageFunc compositeImage);

    void renderPlan(
      doc::RenderPlan& plan,
      Image* image,
      const gfx::Clip& area,
      const frame_t frame,
      const CompositeImageFunc compositeImage,
      const bool render_background,
      const bool render_transparent,
      const BlendMode blendMode);

    void renderCel(
      Image* dst_image,
      const Cel* cel,
      const Image* cel_image,
      const Layer* cel_layer,
      const Palette* pal,
      const gfx::RectF& celBounds,
      const gfx::Clip& area,
      const CompositeImageFunc compositeImage,
      const int opacity,
      const BlendMode blendMode);

    void renderImage(
      Image* dst_image,
      const Image* cel_image,
      const Palette* pal,
      const gfx::RectF& celBounds,
      const gfx::Clip& area,
      const CompositeImageFunc compositeImage,
      const int opacity,
      const BlendMode blendMode);

    CompositeImageFunc getImageComposition(
      const PixelFormat dstFormat,
      const PixelFormat srcFormat,
      const Layer* layer);

    bool checkIfWeShouldUsePreview(const Cel* cel) const;

    int m_flags;
    int m_nonactiveLayersOpacity;
    const Sprite* m_sprite;
    const Layer* m_currentLayer;
    frame_t m_currentFrame;
    Projection m_proj;
    ExtraType m_extraType;
    const Cel* m_extraCel;
    const Image* m_extraImage;
    BlendMode m_extraBlendMode;
    bool m_newBlendMethod;
    BgOptions m_bg;
    int m_globalOpacity;
    const Layer* m_selectedLayerForOpacity;
    const Layer* m_selectedLayer;
    frame_t m_selectedFrame;
    const Image* m_previewImage;
    const Tileset* m_previewTileset;
    gfx::Point m_previewPos;
    BlendMode m_previewBlendMode;
    OnionskinOptions m_onionskin;
    ImageBufferPtr m_tmpBuf;
  };

  void composite_image(Image* dst,
                       const Image* src,
                       const Palette* pal,
                       const int x,
                       const int y,
                       const int opacity,
                       const BlendMode blendMode);

} // namespace render

#endif
