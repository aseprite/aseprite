// Aseprite
// Copyright (C) 2022  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/render/simple_renderer.h"

namespace app {

void SimpleRenderer::setRefLayersVisiblity(const bool visible)
{
  m_render.setRefLayersVisiblity(visible);
}

void SimpleRenderer::setNonactiveLayersOpacity(const int opacity)
{
  m_render.setNonactiveLayersOpacity(opacity);
}

void SimpleRenderer::setNewBlendMethod(const bool newBlend)
{
  m_render.setNewBlend(newBlend);
}

void SimpleRenderer::setBgOptions(const render::BgOptions& bg)
{
  m_render.setBgOptions(bg);
}

void SimpleRenderer::setProjection(const render::Projection& projection)
{
  m_render.setProjection(projection);
}

void SimpleRenderer::setSelectedLayer(const doc::Layer* layer)
{
  m_render.setSelectedLayer(layer);
}

void SimpleRenderer::setPreviewImage(const doc::Layer* layer,
                                     const doc::frame_t frame,
                                     const doc::Image* image,
                                     const doc::Tileset* tileset,
                                     const gfx::Point& pos,
                                     const doc::BlendMode blendMode)
{
  m_render.setPreviewImage(layer, frame, image, tileset,
                           pos, blendMode);
}

void SimpleRenderer::removePreviewImage()
{
  m_render.removePreviewImage();
}

void SimpleRenderer::setExtraImage(render::ExtraType type,
                                   const doc::Cel* cel,
                                   const doc::Image* image,
                                   const doc::BlendMode blendMode,
                                   const doc::Layer* currentLayer,
                                   const doc::frame_t currentFrame)
{
  m_render.setExtraImage(type, cel, image, blendMode,
                         currentLayer, currentFrame);
}

void SimpleRenderer::removeExtraImage()
{
  m_render.removeExtraImage();
}

void SimpleRenderer::setOnionskin(const render::OnionskinOptions& options)
{
  m_render.setOnionskin(options);
}

void SimpleRenderer::disableOnionskin()
{
  m_render.disableOnionskin();
}

void SimpleRenderer::renderSprite(doc::Image* dstImage,
                                  const doc::Sprite* sprite,
                                  const doc::frame_t frame)
{
  m_render.renderSprite(dstImage, sprite, frame);
}

void SimpleRenderer::renderSprite(doc::Image* dstImage,
                                  const doc::Sprite* sprite,
                                  const doc::frame_t frame,
                                  const gfx::ClipF& area)
{
  m_render.renderSprite(dstImage, sprite, frame, area);
}

void SimpleRenderer::renderCheckeredBackground(doc::Image* dstImage,
                                               const gfx::Clip& area)
{
  m_render.renderCheckeredBackground(dstImage, area);
}

void SimpleRenderer::renderImage(doc::Image* dstImage,
                                 const doc::Image* srcImage,
                                 const doc::Palette* pal,
                                 const int x,
                                 const int y,
                                 const int opacity,
                                 const doc::BlendMode blendMode)
{
  m_render.renderImage(dstImage, srcImage, pal,
                       x, y, opacity, blendMode);
}

} // namespace app
