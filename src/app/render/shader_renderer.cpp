// Aseprite
// Copyright (C) 2022-2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/render/shader_renderer.h"

#if SK_ENABLE_SKSL && ENABLE_DEVMODE // TODO remove ENABLE_DEVMODE when the ShaderRenderer is ready

  #include "app/color_utils.h"
  #include "app/util/shader_helpers.h"
  #include "doc/render_plan.h"
  #include "os/skia/skia_surface.h"

  #include "include/core/SkCanvas.h"
  #include "include/effects/SkRuntimeEffect.h"

namespace app {

using namespace doc;

namespace {

const char* kBgShaderCode = R"(
uniform half4 iBg1, iBg2;
uniform half2 iStripeSize;

half4 main(vec2 fragcoord) {
 vec2 u = fragcoord.xy / iStripeSize.xy;
 return (mod(mod(floor(u.x), 2) + mod(floor(u.y), 2), 2) != 0.0 ? iBg2: iBg1);
}
)";

const char* kIndexedShaderCode = R"(
uniform shader iImg;
uniform shader iPal;

half4 main(vec2 fragcoord) {
 int index = int(255.0 * iImg.eval(fragcoord).a);
 return iPal.eval(half2(index, 0));
}
)";

const char* kGrayscaleShaderCode = R"(
uniform shader iImg;

half4 main(vec2 fragcoord) {
 half4 c = iImg.eval(fragcoord);
 return half4(c.rrr * c.g, c.g);
}
)";

inline SkBlendMode to_skia(const doc::BlendMode bm)
{
  switch (bm) {
    case doc::BlendMode::NORMAL:         return SkBlendMode::kSrcOver;
    case doc::BlendMode::MULTIPLY:       return SkBlendMode::kMultiply;
    case doc::BlendMode::SCREEN:         return SkBlendMode::kScreen;
    case doc::BlendMode::OVERLAY:        return SkBlendMode::kOverlay;
    case doc::BlendMode::DARKEN:         return SkBlendMode::kDarken;
    case doc::BlendMode::LIGHTEN:        return SkBlendMode::kLighten;
    case doc::BlendMode::COLOR_DODGE:    return SkBlendMode::kColorDodge;
    case doc::BlendMode::COLOR_BURN:     return SkBlendMode::kColorBurn;
    case doc::BlendMode::HARD_LIGHT:     return SkBlendMode::kHardLight;
    case doc::BlendMode::SOFT_LIGHT:     return SkBlendMode::kSoftLight;
    case doc::BlendMode::DIFFERENCE:     return SkBlendMode::kDifference;
    case doc::BlendMode::EXCLUSION:      return SkBlendMode::kExclusion;
    case doc::BlendMode::HSL_HUE:        return SkBlendMode::kHue;
    case doc::BlendMode::HSL_SATURATION: return SkBlendMode::kSaturation;
    case doc::BlendMode::HSL_COLOR:      return SkBlendMode::kColor;
    case doc::BlendMode::HSL_LUMINOSITY: return SkBlendMode::kLuminosity;
    case doc::BlendMode::ADDITION:       return SkBlendMode::kPlus;
    case doc::BlendMode::SUBTRACT:       break; // TODO
    case doc::BlendMode::DIVIDE:         break;         // TODO
  }
  return SkBlendMode::kSrc;
}

} // anonymous namespace

ShaderRenderer::ShaderRenderer()
{
  m_properties.renderBgOnScreen = true;
  m_properties.requiresRgbaBackbuffer = true;

  m_bgEffect = make_shader(kBgShaderCode);
  m_indexedEffect = make_shader(kIndexedShaderCode);
  m_grayscaleEffect = make_shader(kGrayscaleShaderCode);
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
  m_sprite = sprite;

  // Copy the current color palette to a 256 palette (so all entries
  // outside the valid range will be transparent in the kIndexedShaderCode)
  if (m_sprite->pixelFormat() == IMAGE_INDEXED) {
    const auto srcPal = m_sprite->palette(frame);
    m_palette.resize(256, 0);
    for (int i = 0; i < srcPal->size(); ++i)
      m_palette.setEntry(i, srcPal->entry(i));

    m_bgLayer = sprite->backgroundLayer();
    if (!m_bgLayer || !m_bgLayer->isVisible()) {
      afterBackgroundLayerIsPainted();
    }
  }
  else {
    m_bgLayer = nullptr;
  }

  SkCanvas* canvas = &static_cast<os::SkiaSurface*>(dstSurface)->canvas();
  canvas->save();
  {
    SkPaint p;
    p.setStyle(SkPaint::kFill_Style);
    p.setColor(SK_ColorTRANSPARENT);
    p.setBlendMode(SkBlendMode::kSrc);
    canvas->drawRect(SkRect::MakeXYWH(area.dst.x, area.dst.y, area.size.w, area.size.h), p);

    // Draw cels
    canvas->translate(area.dst.x - area.src.x, area.dst.y - area.src.y);
    canvas->scale(m_proj.scaleX(), m_proj.scaleY());

    RenderPlan plan;
    plan.addLayer(sprite->root(), frame);
    renderPlan(canvas, sprite, plan, frame, area);
  }
  canvas->restore();
}

void ShaderRenderer::renderPlan(SkCanvas* canvas,
                                const doc::Sprite* sprite,
                                const doc::RenderPlan& plan,
                                const doc::frame_t frame,
                                const gfx::ClipF& area)
{
  for (const auto& item : plan.items()) {
    const Cel* cel = item.cel;
    const Layer* layer = item.layer;

    switch (layer->type()) {
      case doc::ObjectType::LayerImage: {
        auto imgLayer = static_cast<const LayerImage*>(layer);

        if (cel) {
          const doc::Image* celImage = nullptr;
          gfx::RectF celBounds;

          // Is the 'm_previewImage' set to be used with this layer?
          if (m_previewImage && checkIfWeShouldUsePreview(cel)) {
            celImage = m_previewImage;
            celBounds = gfx::RectF(m_previewPos.x,
                                   m_previewPos.y,
                                   m_previewImage->width(),
                                   m_previewImage->height());
          }
          // If not, we use the original cel-image from the images' stock
          else {
            celImage = cel->image();
            if (layer->isReference())
              celBounds = cel->boundsF();
            else
              celBounds = cel->bounds();
          }

          int t;
          int opacity = cel->opacity();
          opacity = MUL_UN8(opacity, imgLayer->opacity(), t);

          drawImage(canvas, celImage, celBounds.x, celBounds.y, opacity, imgLayer->blendMode());
        }
        break;
      }

      case doc::ObjectType::LayerTilemap: {
        auto tilemapLayer = static_cast<const LayerTilemap*>(layer);

        // TODO selected layer?
        if (!cel)
          break;

        const gfx::RectF celBounds = cel->bounds();
        const doc::Image* celImage = cel->image();

        doc::Grid grid = tilemapLayer->tileset()->grid();
        grid.origin(grid.origin() + gfx::Point(celBounds.origin()));

        // Is the 'm_previewTileset' set to be used with this layer?
        const Tileset* tileset;
        if (m_previewTileset && cel && checkIfWeShouldUsePreview(cel)) {
          tileset = m_previewTileset;
        }
        else {
          tileset = tilemapLayer->tileset();
          ASSERT(tileset);
          if (!tileset)
            return;
        }

        const gfx::Clip iarea(area);
        gfx::Rect tilesToDraw = grid.canvasToTile(m_proj.remove(gfx::Rect(iarea.src, iarea.size)));

        int yPixelsPerTile = m_proj.applyY(grid.tileSize().h);
        if (yPixelsPerTile > 0 && (iarea.size.h + iarea.src.y) % yPixelsPerTile > 0)
          tilesToDraw.h += 1;
        int xPixelsPerTile = m_proj.applyX(grid.tileSize().w);
        if (xPixelsPerTile > 0 && (iarea.size.w + iarea.src.x) % xPixelsPerTile > 0)
          tilesToDraw.w += 1;

        // As area.size is not empty at this point, we have to draw at
        // least one tile (and the clipping will be performed for the
        // tile pixels later).
        if (tilesToDraw.w < 1)
          tilesToDraw.w = 1;
        if (tilesToDraw.h < 1)
          tilesToDraw.h = 1;

        tilesToDraw &= celImage->bounds();

        for (int v = tilesToDraw.y; v < tilesToDraw.y2(); ++v) {
          for (int u = tilesToDraw.x; u < tilesToDraw.x2(); ++u) {
            auto tileBoundsOnCanvas = grid.tileToCanvas(gfx::Rect(u, v, 1, 1));
            if (!celImage->bounds().contains(u, v))
              continue;

            const tile_t t = celImage->getPixel(u, v);
            if (t != doc::notile) {
              const tile_index i = tile_geti(t);
              const ImageRef tileImage = tileset->get(i);
              if (!tileImage)
                continue;

              int t;
              int opacity = cel->opacity();
              opacity = MUL_UN8(opacity, tilemapLayer->opacity(), t);

              drawImage(canvas,
                        tileImage.get(),
                        tileBoundsOnCanvas.x,
                        tileBoundsOnCanvas.y,
                        opacity,
                        tilemapLayer->blendMode());
            }
          }
        }
        break;
      }

      case doc::ObjectType::LayerGroup: ASSERT(false); break;
    }

    if (layer == m_bgLayer) {
      afterBackgroundLayerIsPainted();
    }
  }
}

void ShaderRenderer::renderCheckeredBackground(os::Surface* dstSurface,
                                               const doc::Sprite* sprite,
                                               const gfx::Clip& area)
{
  SkRuntimeShaderBuilder builder(m_bgEffect);
  builder.uniform("iBg1") = gfxColor_to_SkV4(color_utils::color_for_ui(
    app::Color::fromImage(m_bgOptions.colorPixelFormat, m_bgOptions.color1)));
  builder.uniform("iBg2") = gfxColor_to_SkV4(color_utils::color_for_ui(
    app::Color::fromImage(m_bgOptions.colorPixelFormat, m_bgOptions.color2)));

  float sx = (m_bgOptions.zoom ? m_proj.scaleX() : 1.0);
  float sy = (m_bgOptions.zoom ? m_proj.scaleY() : 1.0);

  builder.uniform("iStripeSize") = SkV2{ float(m_bgOptions.stripeSize.w) * sx,
                                         float(m_bgOptions.stripeSize.h) * sy };

  SkCanvas* canvas = &static_cast<os::SkiaSurface*>(dstSurface)->canvas();
  canvas->save();
  {
    SkPaint p;
    p.setStyle(SkPaint::kFill_Style);
    p.setShader(builder.makeShader());

    canvas->translate(SkIntToScalar(area.dst.x - area.src.x),
                      SkIntToScalar(area.dst.y - area.src.y));
    canvas->drawRect(SkRect::MakeXYWH(area.src.x, area.src.y, area.size.w, area.size.h), p);
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

void ShaderRenderer::drawImage(SkCanvas* canvas,
                               const doc::Image* srcImage,
                               const int x,
                               const int y,
                               const int opacity,
                               const doc::BlendMode blendMode)
{
  auto skImg = make_skimage_for_docimage(srcImage);

  switch (srcImage->colorMode()) {
    case doc::ColorMode::RGB: {
      SkPaint p;
      p.setAlpha(opacity);
      p.setBlendMode(to_skia(blendMode));
      canvas->drawImage(skImg.get(), SkIntToScalar(x), SkIntToScalar(y), SkSamplingOptions(), &p);
      break;
    }

    case doc::ColorMode::GRAYSCALE: {
      SkRuntimeShaderBuilder builder(m_grayscaleEffect);
      builder.child("iImg") = skImg->makeRawShader(SkSamplingOptions(SkFilterMode::kNearest));

      SkPaint p;
      p.setAlpha(opacity);
      p.setBlendMode(to_skia(blendMode));
      p.setStyle(SkPaint::kFill_Style);
      p.setShader(builder.makeShader());

      canvas->save();
      canvas->translate(SkIntToScalar(x), SkIntToScalar(y));
      canvas->drawRect(SkRect::MakeXYWH(0, 0, srcImage->width(), srcImage->height()), p);
      canvas->restore();
      break;
    }

    case doc::ColorMode::INDEXED: {
      // Use the palette data as an "width x height" image where
      // width=number of palette colors, and height=1
      const size_t palSize = sizeof(color_t) * m_palette.size();
      auto skPalData = SkData::MakeWithoutCopy((const void*)m_palette.rawColorsData(), palSize);
      auto skPal = SkImage::MakeRasterData(
        SkImageInfo::Make(m_palette.size(), 1, kRGBA_8888_SkColorType, kUnpremul_SkAlphaType),
        skPalData,
        palSize);

      SkRuntimeShaderBuilder builder(m_indexedEffect);
      builder.child("iImg") = skImg->makeRawShader(SkSamplingOptions(SkFilterMode::kNearest));
      builder.child("iPal") = skPal->makeShader(SkSamplingOptions(SkFilterMode::kNearest));

      SkPaint p;
      p.setAlpha(opacity);
      p.setBlendMode(to_skia(blendMode));
      p.setStyle(SkPaint::kFill_Style);
      p.setShader(builder.makeShader());

      canvas->save();
      canvas->translate(SkIntToScalar(x), SkIntToScalar(y));
      canvas->drawRect(SkRect::MakeXYWH(0, 0, srcImage->width(), srcImage->height()), p);
      canvas->restore();
      break;
    }
  }
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

void ShaderRenderer::afterBackgroundLayerIsPainted()
{
  if (m_sprite && m_sprite->pixelFormat() == IMAGE_INDEXED) {
    // Only after we draw the background layer we set the transparent
    // index of the sprite as transparent, so the shader can return
    // the transparent color for this specific index on transparent
    // layers.
    m_palette.setEntry(m_sprite->transparentColor(), 0);
  }
}

} // namespace app

#endif // SK_ENABLE_SKSL && ENABLE_DEVMODE
