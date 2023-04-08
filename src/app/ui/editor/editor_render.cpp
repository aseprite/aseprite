// Aseprite
// Copyright (C) 2019-2023  Igara Studio S.A.
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
#include "app/render/shader_renderer.h"
#include "app/render/simple_renderer.h"

namespace app {

static doc::ImageBufferPtr g_renderBuffer;

EditorRender::EditorRender()
  // TODO create a switch in the preferences
  : m_renderer(std::make_unique<SimpleRenderer>())
{
  m_renderer->setNewBlendMethod(
    Preferences::instance().experimental.newBlend());
}

EditorRender::~EditorRender()
{
}

EditorRender::Type EditorRender::type() const
{
#if SK_ENABLE_SKSL && ENABLE_DEVMODE
  if (dynamic_cast<ShaderRenderer*>(m_renderer.get()))
    return Type::kShaderRenderer;
#endif
  return Type::kSimpleRenderer;
}

void EditorRender::setType(const Type type)
{
#if SK_ENABLE_SKSL && ENABLE_DEVMODE
  if (type == Type::kShaderRenderer) {
    m_renderer = std::make_unique<ShaderRenderer>();
  }
  else
#endif
  {
    m_renderer = std::make_unique<SimpleRenderer>();
  }

  m_renderer->setNewBlendMethod(
    Preferences::instance().experimental.newBlend());
}

void EditorRender::setRefLayersVisiblity(const bool visible)
{
  m_renderer->setRefLayersVisiblity(visible);
}

void EditorRender::setNonactiveLayersOpacity(const int opacity)
{
  m_renderer->setNonactiveLayersOpacity(opacity);
}

void EditorRender::setNewBlendMethod(const bool newBlend)
{
  m_renderer->setNewBlendMethod(newBlend);
}

void EditorRender::setProjection(const render::Projection& projection)
{
  m_renderer->setProjection(projection);
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
  bg.colorPixelFormat = pixelFormat;
  bg.color1 = color_utils::color_for_image_without_alpha(docPref.bg.color1(), pixelFormat);
  bg.color2 = color_utils::color_for_image_without_alpha(docPref.bg.color2(), pixelFormat);
  bg.stripeSize = tile;
  m_renderer->setBgOptions(bg);
}

void EditorRender::setTransparentBackground()
{
  m_renderer->setBgOptions(render::BgOptions::MakeTransparent());
}

void EditorRender::setSelectedLayer(const doc::Layer* layer)
{
  m_renderer->setSelectedLayer(layer);
}

void EditorRender::setPreviewImage(const doc::Layer* layer,
                                   const doc::frame_t frame,
                                   const doc::Image* image,
                                   const doc::Tileset* tileset,
                                   const gfx::Point& pos,
                                   const doc::BlendMode blendMode)
{
  m_renderer->setPreviewImage(layer, frame, image, tileset,
                              pos, blendMode);
}

void EditorRender::removePreviewImage()
{
  m_renderer->removePreviewImage();
}

void EditorRender::setExtraImage(
  render::ExtraType type,
  const doc::Cel* cel,
  const doc::Image* image,
  doc::BlendMode blendMode,
  const doc::Layer* currentLayer,
  doc::frame_t currentFrame)
{
  m_renderer->setExtraImage(type, cel, image, blendMode,
                          currentLayer, currentFrame);
}

void EditorRender::removeExtraImage()
{
  m_renderer->removeExtraImage();
}

void EditorRender::setOnionskin(const render::OnionskinOptions& options)
{
  m_renderer->setOnionskin(options);
}

void EditorRender::disableOnionskin()
{
  m_renderer->disableOnionskin();
}

void EditorRender::renderSprite(
  os::Surface* dstSurface,
  const doc::Sprite* sprite,
  doc::frame_t frame,
  const gfx::ClipF& area)
{
  m_renderer->renderSprite(dstSurface, sprite, frame, area);
}

void EditorRender::renderCheckeredBackground(
  os::Surface* dstSurface,
  const doc::Sprite* sprite,
  const gfx::Clip& area)
{
  m_renderer->renderCheckeredBackground(dstSurface, sprite, area);
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
  m_renderer->renderImage(dst_image, src_image, pal,
                          x, y, opacity, blendMode);
}
// static
doc::ImageBufferPtr EditorRender::getRenderImageBuffer()
{
  if (!g_renderBuffer)
    g_renderBuffer.reset(new doc::ImageBuffer);
  return g_renderBuffer;
}

} // namespace app
