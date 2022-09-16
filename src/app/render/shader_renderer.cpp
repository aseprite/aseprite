// Aseprite
// Copyright (C) 2022  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/render/shader_renderer.h"

#if SK_ENABLE_SKSL

#include "app/color_utils.h"
#include "app/util/shader_helpers.h"
#include "os/skia/skia_surface.h"

#include "include/core/SkCanvas.h"
#include "include/effects/SkRuntimeEffect.h"

namespace app {

static const char* kBgShaderCode = R"(
uniform half3 iRes, iCanvas, iSrcPos;
uniform half4 iBg1, iBg2;
uniform half2 iStripeSize;

half4 main(vec2 fragcoord) {
 vec2 u = (iSrcPos.xy+fragcoord.xy) / iStripeSize.xy;
 return (mod(mod(floor(u.x), 2) + mod(floor(u.y), 2), 2) != 0.0 ? iBg2: iBg1);
}
)";

ShaderRenderer::ShaderRenderer()
{
  auto result = SkRuntimeEffect::MakeForShader(SkString(kBgShaderCode));
  if (!result.errorText.isEmpty()) {
    LOG(ERROR, "Shader error: %s\n", result.errorText.c_str());
    std::printf("Shader error: %s\n", result.errorText.c_str());
    throw std::runtime_error("Cannot compile shaders for ShaderRenderer");
  }
  m_bgEffect = result.effect;
}

ShaderRenderer::~ShaderRenderer() = default;

void ShaderRenderer::setRefLayersVisiblity(const bool visible)
{
  // TODO impl
}

void ShaderRenderer::setNonactiveLayersOpacity(const int opacity)
{
  // TODO impl
}

void ShaderRenderer::setNewBlendMethod(const bool newBlend)
{
  // TODO impl
}

void ShaderRenderer::setBgOptions(const render::BgOptions& bg)
{
  m_bgOptions = bg;
}

void ShaderRenderer::setProjection(const render::Projection& projection)
{
  // TODO impl
}

void ShaderRenderer::setSelectedLayer(const doc::Layer* layer)
{
  // TODO impl
}

void ShaderRenderer::setPreviewImage(const doc::Layer* layer,
                                     const doc::frame_t frame,
                                     const doc::Image* image,
                                     const doc::Tileset* tileset,
                                     const gfx::Point& pos,
                                     const doc::BlendMode blendMode)
{
  // TODO impl
}

void ShaderRenderer::removePreviewImage()
{
  // TODO impl
}

void ShaderRenderer::setExtraImage(render::ExtraType type,
                                   const doc::Cel* cel,
                                   const doc::Image* image,
                                   const doc::BlendMode blendMode,
                                   const doc::Layer* currentLayer,
                                   const doc::frame_t currentFrame)
{
  // TODO impl
}

void ShaderRenderer::removeExtraImage()
{
  // TODO impl
}

void ShaderRenderer::setOnionskin(const render::OnionskinOptions& options)
{
  // TODO impl
}

void ShaderRenderer::disableOnionskin()
{
  // TODO impl
}

void ShaderRenderer::renderSprite(doc::Image* dstImage,
                                  const doc::Sprite* sprite,
                                  const doc::frame_t frame)
{
  // TODO impl
}

void ShaderRenderer::renderSprite(os::Surface* dstSurface,
                                  const doc::Sprite* sprite,
                                  const doc::frame_t frame,
                                  const gfx::ClipF& area)
{
  SkRuntimeShaderBuilder builder(m_bgEffect);
  builder.uniform("iRes") = SkV3{float(area.size.w), float(area.size.h), 0.0f};
  builder.uniform("iCanvas") = SkV3{float(sprite->width()), float(sprite->height()), 0.0f};
  builder.uniform("iSrcPos") = SkV3{float(area.src.x), float(area.src.y), 0.0f};
  builder.uniform("iBg1") = gfxColor_to_SkV4(
    color_utils::color_for_ui(
      app::Color::fromImage(sprite->pixelFormat(),
                            m_bgOptions.color1)));
  builder.uniform("iBg2") = gfxColor_to_SkV4(
    color_utils::color_for_ui(
      app::Color::fromImage(sprite->pixelFormat(),
                            m_bgOptions.color2)));
  builder.uniform("iStripeSize") = SkV2{
    float(m_bgOptions.stripeSize.w),
    float(m_bgOptions.stripeSize.h)};

  SkCanvas* canvas = &static_cast<os::SkiaSurface*>(dstSurface)->canvas();
  canvas->save();
  {
    SkPaint p;
    p.setStyle(SkPaint::kFill_Style);
    p.setShader(builder.makeShader());

    canvas->drawRect(SkRect::MakeXYWH(area.dst.x, area.dst.y, area.size.w, area.size.h), p);

    // Draw cels
    drawLayerGroup(canvas, sprite, sprite->root(), frame, area);
  }
  canvas->restore();
}

void ShaderRenderer::drawLayerGroup(SkCanvas* canvas,
                                    const doc::Sprite* sprite,
                                    const doc::LayerGroup* group,
                                    const doc::frame_t frame,
                                    const gfx::ClipF& area)
{
  for (auto layer : group->layers()) {
    switch (layer->type()) {

      case doc::ObjectType::LayerImage: {
        auto imageLayer = static_cast<const LayerImage*>(layer);

        if (doc::Cel* cel = imageLayer->cel(frame)) {
          doc::Image* image = cel->image();

          auto skData = SkData::MakeWithoutCopy(
            (const void*)image->getPixelAddress(0, 0),
            image->getMemSize());

          // TODO support other color modes
          ASSERT(image->colorMode() == doc::ColorMode::RGB);

          auto skImg = SkImage::MakeRasterData(
            SkImageInfo::Make(image->width(),
                              image->height(),
                              kRGBA_8888_SkColorType,
                              kUnpremul_SkAlphaType),
            skData,
            image->getRowStrideSize());

          sk_sp<SkShader> imgShader = skImg->makeShader(SkSamplingOptions(SkFilterMode::kLinear));

          SkPaint p;
          p.setStyle(SkPaint::kFill_Style);
          p.setShader(imgShader);

          canvas->save();
          canvas->translate(SkIntToScalar(area.dst.x + cel->x() - area.src.x),
                            SkIntToScalar(area.dst.y + cel->y() - area.src.y));
          canvas->drawRect(SkRect::MakeXYWH(0, 0, image->width(), image->height()), p);
          canvas->restore();
        }
        break;
      }

      case doc::ObjectType::LayerTilemap:
        // TODO impl
        break;

      case doc::ObjectType::LayerGroup:
        // TODO draw a group in a sub-surface and then compose that surface
        drawLayerGroup(canvas, sprite,
                       static_cast<const doc::LayerGroup*>(layer),
                       frame, area);
        break;
    }
  }
}

void ShaderRenderer::renderCheckeredBackground(doc::Image* dstImage,
                                               const gfx::Clip& area)
{
  // TODO impl
}

void ShaderRenderer::renderImage(doc::Image* dstImage,
                                 const doc::Image* srcImage,
                                 const doc::Palette* pal,
                                 const int x,
                                 const int y,
                                 const int opacity,
                                 const doc::BlendMode blendMode)
{
  // TODO impl
}

} // namespace app

#endif // SK_ENABLE_SKSL
