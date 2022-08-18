// Aseprite
// Copyright (C) 2019-2022  Igara Studio S.A.
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/editor_render.h"

#include "app/color_utils.h"
#include "app/pref/preferences.h"
#include "render/render.h"

namespace app {

static doc::ImageBufferPtr g_renderBuffer;

EditorRender::EditorRender()
  : m_render(new render::Render)
{
  m_render->setNewBlend(Preferences::instance().experimental.newBlend());
}

EditorRender::~EditorRender()
{
  delete m_render;
}

void EditorRender::setRefLayersVisiblity(const bool visible)
{
  m_render->setRefLayersVisiblity(visible);
}

void EditorRender::setNonactiveLayersOpacity(const int opacity)
{
  m_render->setNonactiveLayersOpacity(opacity);
}

void EditorRender::setNewBlendMethod(const bool newBlend)
{
  m_render->setNewBlend(newBlend);
}

void EditorRender::setProjection(const render::Projection& projection)
{
  m_render->setProjection(projection);
}

void EditorRender::setupBackground(Doc* doc, doc::PixelFormat pixelFormat)
{
  DocumentPreferences& docPref = Preferences::instance().document(doc);
  render::BgType bgType;

  gfx::Size tile;
  switch (docPref.bg.type()) {
    case app::gen::BgType::CHECKERED_16x16:
      bgType = render::BgType::CHECKERED;
      tile = gfx::Size(16, 16);
      break;
    case app::gen::BgType::CHECKERED_8x8:
      bgType = render::BgType::CHECKERED;
      tile = gfx::Size(8, 8);
      break;
    case app::gen::BgType::CHECKERED_4x4:
      bgType = render::BgType::CHECKERED;
      tile = gfx::Size(4, 4);
      break;
    case app::gen::BgType::CHECKERED_2x2:
      bgType = render::BgType::CHECKERED;
      tile = gfx::Size(2, 2);
      break;
    case app::gen::BgType::CHECKERED_1x1:
      bgType = render::BgType::CHECKERED;
      tile = gfx::Size(1, 1);
      break;
    case app::gen::BgType::CHECKERED_CUSTOM:
      bgType = render::BgType::CHECKERED;
      tile = docPref.bg.size();
      break;
    default:
      bgType = render::BgType::TRANSPARENT;
      break;
  }

  render::BgOptions bg;
  bg.type = bgType;
  bg.zoom = docPref.bg.zoom();
  bg.color1 = color_utils::color_for_image_without_alpha(docPref.bg.color1(), pixelFormat);
  bg.color2 = color_utils::color_for_image_without_alpha(docPref.bg.color2(), pixelFormat);
  bg.stripeSize = tile;
  m_render->setBgOptions(bg);
}

void EditorRender::setTransparentBackground()
{
  m_render->setBgOptions(render::BgOptions::MakeTransparent());
}

void EditorRender::setSelectedLayer(const doc::Layer* layer)
{
  m_render->setSelectedLayer(layer);
}

void EditorRender::setPreviewImage(const doc::Layer* layer,
                                   const doc::frame_t frame,
                                   const doc::Image* image,
                                   const doc::Tileset* tileset,
                                   const gfx::Point& pos,
                                   const doc::BlendMode blendMode)
{
  m_render->setPreviewImage(layer, frame, image, tileset,
                            pos, blendMode);
}

void EditorRender::removePreviewImage()
{
  m_render->removePreviewImage();
}

void EditorRender::setExtraImage(
  render::ExtraType type,
  const doc::Cel* cel,
  const doc::Image* image,
  doc::BlendMode blendMode,
  const doc::Layer* currentLayer,
  doc::frame_t currentFrame)
{
  m_render->setExtraImage(type, cel, image, blendMode,
                          currentLayer, currentFrame);
}

void EditorRender::removeExtraImage()
{
  m_render->removeExtraImage();
}

void EditorRender::setOnionskin(const render::OnionskinOptions& options)
{
  m_render->setOnionskin(options);
}

void EditorRender::disableOnionskin()
{
  m_render->disableOnionskin();
}

void EditorRender::renderSprite(
  doc::Image* dstImage,
  const doc::Sprite* sprite,
  doc::frame_t frame)
{
  m_render->renderSprite(dstImage, sprite, frame);
}

void EditorRender::renderSprite(
  doc::Image* dstImage,
  const doc::Sprite* sprite,
  doc::frame_t frame,
  const gfx::ClipF& area)
{
  m_render->renderSprite(dstImage, sprite, frame, area);
}

void EditorRender::renderCheckeredBackground(
  doc::Image* image,
  const gfx::Clip& area)
{
  m_render->renderCheckeredBackground(image, area);
}

void EditorRender::renderImage(
  doc::Image* dst_image,
  const doc::Image* src_image,
  const doc::Palette* pal,
  const int x,
  const int y,
  const int opacity,
  const doc::BlendMode blendMode)
{
  m_render->renderImage(dst_image, src_image, pal,
                        x, y, opacity, blendMode);
}

doc::ImageBufferPtr EditorRender::getRenderImageBuffer()
{
  if (!g_renderBuffer)
    g_renderBuffer.reset(new doc::ImageBuffer);
  return g_renderBuffer;
}

} // namespace app
