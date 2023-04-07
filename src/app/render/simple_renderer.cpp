// Aseprite
// Copyright (C) 2022-2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/render/simple_renderer.h"

#include "app/ui/editor/editor_render.h"
#include "app/util/conversion_to_surface.h"

namespace app {

using namespace doc;

SimpleRenderer::SimpleRenderer()
{
  m_properties.outputsUnpremultiplied = true;
}

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

void SimpleRenderer::renderSprite(os::Surface* dstSurface,
                                  const doc::Sprite* sprite,
                                  const doc::frame_t frame,
                                  const gfx::ClipF& area)
{
  ImageRef dstImage(Image::create(
                      IMAGE_RGB, area.size.w, area.size.h,
                      EditorRender::getRenderImageBuffer()));
  m_render.renderSprite(dstImage.get(), sprite, frame, area);

  convert_image_to_surface(dstImage.get(), sprite->palette(frame),
                           dstSurface, 0, 0, 0, 0, area.size.w, area.size.h);
}

void SimpleRenderer::renderCheckeredBackground(os::Surface* dstSurface,
                                               const doc::Sprite* sprite,
                                               const gfx::Clip& area)
{
  ImageRef dstImage(Image::create(
                      IMAGE_RGB, area.size.w, area.size.h,
                      EditorRender::getRenderImageBuffer()));

  m_render.renderCheckeredBackground(dstImage.get(), area);

  convert_image_to_surface(dstImage.get(), sprite->palette(0),
                           dstSurface, 0, 0, 0, 0, area.size.w, area.size.h);
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
