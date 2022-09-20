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

namespace {

const char* kBgShaderCode = R"(
uniform half4 iBg1, iBg2;
uniform half2 iStripeSize;

half4 main(vec2 fragcoord) {
 vec2 u = fragcoord.xy / iStripeSize.xy;
 return (mod(mod(floor(u.x), 2) + mod(floor(u.y), 2), 2) != 0.0 ? iBg2: iBg1);
}
)";

inline SkBlendMode to_skia(const doc::BlendMode bm) {
  switch (bm) {
    case doc::BlendMode::NORMAL: return SkBlendMode::kSrcOver;
    case doc::BlendMode::MULTIPLY: return SkBlendMode::kMultiply;
    case doc::BlendMode::SCREEN: return SkBlendMode::kScreen;
    case doc::BlendMode::OVERLAY: return SkBlendMode::kOverlay;
    case doc::BlendMode::DARKEN: return SkBlendMode::kDarken;
    case doc::BlendMode::LIGHTEN: return SkBlendMode::kLighten;
    case doc::BlendMode::COLOR_DODGE: return SkBlendMode::kColorDodge;
    case doc::BlendMode::COLOR_BURN: return SkBlendMode::kColorBurn;
    case doc::BlendMode::HARD_LIGHT: return SkBlendMode::kHardLight;
    case doc::BlendMode::SOFT_LIGHT: return SkBlendMode::kSoftLight;
    case doc::BlendMode::DIFFERENCE: return SkBlendMode::kDifference;
    case doc::BlendMode::EXCLUSION: return SkBlendMode::kExclusion;
    case doc::BlendMode::HSL_HUE: return SkBlendMode::kHue;
    case doc::BlendMode::HSL_SATURATION: return SkBlendMode::kSaturation;
    case doc::BlendMode::HSL_COLOR: return SkBlendMode::kColor;
    case doc::BlendMode::HSL_LUMINOSITY: return SkBlendMode::kLuminosity;
    case doc::BlendMode::ADDITION: return SkBlendMode::kPlus;
    case doc::BlendMode::SUBTRACT: break; // TODO
    case doc::BlendMode::DIVIDE: break; // TODO
  }
  return SkBlendMode::kSrc;
}

} // anonymous namespace

ShaderRenderer::ShaderRenderer()
{
  m_properties.renderBgOnScreen = true;
  m_properties.requiresRgbaBackbuffer = true;

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
  m_proj = projection;
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
  m_selectedLayer = layer;
  m_selectedFrame = frame;
  m_previewImage = image;
  m_previewTileset = tileset;
  m_previewPos = pos;
  m_previewBlendMode = blendMode;
}

void ShaderRenderer::removePreviewImage()
{
  m_previewImage = nullptr;
  m_previewTileset = nullptr;
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

void ShaderRenderer::renderSprite(os::Surface* dstSurface,
                                  const doc::Sprite* sprite,
                                  const doc::frame_t frame,
                                  const gfx::ClipF& area)
{
  SkCanvas* canvas = &static_cast<os::SkiaSurface*>(dstSurface)->canvas();
  canvas->save();
  {
    SkPaint p;
    p.setStyle(SkPaint::kFill_Style);
    p.setColor(SK_ColorTRANSPARENT);
    p.setBlendMode(SkBlendMode::kSrc);
    canvas->drawRect(SkRect::MakeXYWH(area.dst.x, area.dst.y, area.size.w, area.size.h), p);

    // Draw cels
    canvas->translate(area.dst.x - area.src.x,
                      area.dst.y - area.src.y);
    canvas->scale(m_proj.scaleX(), m_proj.scaleY());

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
    // Ignore hidden layers
    if (!layer->isVisible())
      continue;

    switch (layer->type()) {

      case doc::ObjectType::LayerImage: {
        auto imgLayer = static_cast<const LayerImage*>(layer);

        if (doc::Cel* cel = imgLayer->cel(frame)) {
          const doc::Image* celImage = nullptr;
          gfx::RectF celBounds;

          // Is the 'm_previewImage' set to be used with this layer?
          if (m_previewImage &&
              checkIfWeShouldUsePreview(cel)) {
            celImage = m_previewImage;
            celBounds = gfx::RectF(m_previewPos.x,
                                   m_previewPos.y,
                                   m_previewImage->width(),
                                   m_previewImage->height());
          }
          // If not, we use the original cel-image from the images' stock
          else {
            celImage = cel->image();
            if (cel->layer()->isReference())
              celBounds = cel->boundsF();
            else
              celBounds = cel->bounds();
          }

          auto skData = SkData::MakeWithoutCopy(
            (const void*)celImage->getPixelAddress(0, 0),
            celImage->getMemSize());

          // TODO support other color modes
          ASSERT(celImage->colorMode() == doc::ColorMode::RGB);

          auto skImg = SkImage::MakeRasterData(
            SkImageInfo::Make(celImage->width(),
                              celImage->height(),
                              kRGBA_8888_SkColorType,
                              kUnpremul_SkAlphaType),
            skData,
            celImage->getRowStrideSize());

          int t;
          int opacity = cel->opacity();
          opacity = MUL_UN8(opacity, imgLayer->opacity(), t);

          SkPaint p;
          p.setAlpha(opacity);
          p.setBlendMode(to_skia(imgLayer->blendMode()));
          canvas->drawImage(skImg.get(),
                            SkIntToScalar(celBounds.x),
                            SkIntToScalar(celBounds.y),
                            SkSamplingOptions(),
                            &p);
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

void ShaderRenderer::renderCheckeredBackground(os::Surface* dstSurface,
                                               const doc::Sprite* sprite,
                                               const gfx::Clip& area)
{
  SkRuntimeShaderBuilder builder(m_bgEffect);
  builder.uniform("iBg1") = gfxColor_to_SkV4(
    color_utils::color_for_ui(
      app::Color::fromImage(sprite->pixelFormat(),
                            m_bgOptions.color1)));
  builder.uniform("iBg2") = gfxColor_to_SkV4(
    color_utils::color_for_ui(
      app::Color::fromImage(sprite->pixelFormat(),
                            m_bgOptions.color2)));

  float sx = (m_bgOptions.zoom ? m_proj.scaleX(): 1.0);
  float sy = (m_bgOptions.zoom ? m_proj.scaleY(): 1.0);

  builder.uniform("iStripeSize") = SkV2{
    float(m_bgOptions.stripeSize.w) * sx,
    float(m_bgOptions.stripeSize.h) * sy};

  SkCanvas* canvas = &static_cast<os::SkiaSurface*>(dstSurface)->canvas();
  canvas->save();
  {
    SkPaint p;
    p.setStyle(SkPaint::kFill_Style);
    p.setShader(builder.makeShader());

    canvas->translate(
      SkIntToScalar(area.dst.x - area.src.x),
      SkIntToScalar(area.dst.y - area.src.y));
    canvas->drawRect(
      SkRect::MakeXYWH(area.src.x, area.src.y, area.size.w, area.size.h), p);
  }
  canvas->restore();
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

// TODO this is equal to Render::checkIfWeShouldUsePreview(const Cel*),
//      we might think in a way to merge both functions
bool ShaderRenderer::checkIfWeShouldUsePreview(const doc::Cel* cel) const
{
  if ((m_selectedLayer == cel->layer())) {
    if (m_selectedFrame == cel->frame()) {
      return true;
    }
    else if (cel->layer()) {
      // This preview might be useful if we are rendering a linked
      // frame to preview.
      Cel* cel2 = cel->layer()->cel(m_selectedFrame);
      if (cel2 && cel2->data() == cel->data())
        return true;
    }
  }
  return false;
}

} // namespace app

#endif // SK_ENABLE_SKSL
