// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "tests/app_test.h"

#include "app/file/file.h"
#include "app/render/shader_renderer.h"
#include "app/render/simple_renderer.h"
#include "app/test_context.h"
#include "doc/blend_funcs.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/image_ref.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "os/surface.h"
#include "os/system.h"

using namespace app;
using namespace doc;

#if SK_ENABLE_SKSL

TEST(RendererTest, MatchRgbaBlenders)
{
  doc::BlendMode blendModes[] = {
    doc::BlendMode::NORMAL,         doc::BlendMode::MULTIPLY,       doc::BlendMode::SCREEN,
    doc::BlendMode::OVERLAY,        doc::BlendMode::DARKEN,         doc::BlendMode::LIGHTEN,
    doc::BlendMode::COLOR_DODGE,    doc::BlendMode::COLOR_BURN,     doc::BlendMode::HARD_LIGHT,
    doc::BlendMode::SOFT_LIGHT,     doc::BlendMode::DIFFERENCE,     doc::BlendMode::EXCLUSION,
    doc::BlendMode::HSL_HUE,        doc::BlendMode::HSL_SATURATION, doc::BlendMode::HSL_COLOR,
    doc::BlendMode::HSL_LUMINOSITY,
  #if 0 // These don't match
    doc::BlendMode::ADDITION,
    doc::BlendMode::SUBTRACT,
    doc::BlendMode::DIVIDE
  #endif
  };

  os::SystemRef system = os::System::make();
  TestContextT<Context> ctx;

  ImageSpec spec(ColorMode::RGB, 256, 256);

  std::unique_ptr<Doc> doc(ctx.documents().add(spec));
  doc->setFilename("__test_renderer_tests__.aseprite");

  Sprite* spr = doc->sprite();
  Layer* backdropLayer = spr->root()->firstLayer();

  Image* backdropImage = backdropLayer->cel(0)->image();
  ImageRef srcImage(Image::createCopy(backdropImage));

  auto it1 = backdropImage->writeArea();
  auto it2 = srcImage->writeArea();
  int y = 0;
  while (it1.nextLine() && it2.nextLine()) {
    uint32_t* p1 = it1.addr32();
    uint32_t* p2 = it2.addr32();
    for (int x = 0; x < spec.width(); ++x, ++p1, ++p2) {
      int u = (x - spec.width() / 2);
      int v = (y - spec.height() / 2);
      int a = 255 - 255 * std::sqrt(u * u + v * v) / (spec.width() / 2);
      *p1 = rgba(x, (x + y) % 256, y, std::clamp(a, 0, 255));
      *p2 = rgba(x, x, x, 255);
    }
    ++y;
  }

  spr->setTotalFrames(sizeof(blendModes) / sizeof(blendModes[0]));
  frame_t frame = 0;
  for (auto blendMode : blendModes) {
    Layer* modeLayer = new LayerImage(spr);
    modeLayer->setBlendMode(blendMode);
    modeLayer->setOpacity(128);
    modeLayer->setName(blend_mode_to_string(blendMode));

    Cel* cel = new Cel(frame, ImageRef(Image::createCopy(srcImage.get())));
    modeLayer->addCel(cel);

    if (frame > 0)
      backdropLayer->addCel(Cel::MakeLink(frame, backdropLayer->cel(0)));

    spr->root()->addLayer(modeLayer);
    ++frame;
  }
  #if 0 // In case you want to see the test image
  save_document(&ctx, doc.get());
  #endif

  SimpleRenderer renderer1;
  ShaderRenderer renderer2;
  os::SurfaceRef render1(system->makeRgbaSurface(spec.width(), spec.height()));
  os::SurfaceRef render2(system->makeRgbaSurface(spec.width(), spec.height()));
  if (!render1 || !render2)
    return;

  renderer1.prepareSpritePalette(spr, 0);
  renderer2.prepareSpritePalette(spr, 0);

  for (frame = 0; frame < spr->totalFrames(); ++frame) {
    auto blendMode = blendModes[(int)frame];

    renderer1.renderSpriteArea(render1.get(),
                               spr,
                               frame,
                               gfx::Clip(0, 0, spec.bounds()),
                               render::Projection());
    renderer2.renderSpriteArea(render2.get(),
                               spr,
                               frame,
                               gfx::Clip(0, 0, spec.bounds()),
                               render::Projection());

    for (int y = 0; y < spec.height(); ++y) {
      auto* p1 = (uint32_t*)render1->getData(0, y);
      auto* p2 = (uint32_t*)render2->getData(0, y);

      for (int x = 0; x < spec.width(); ++x, ++p1, ++p2) {
        color_t a = *p1;
        color_t b = *p2;

        // We allow some error (< 5 per RGB component) between renderers
        ASSERT_TRUE(std::abs(rgba_getr(a) - rgba_getr(b)) < 5 &&
                    std::abs(rgba_getg(a) - rgba_getg(b)) < 5 &&
                    std::abs(rgba_getb(a) - rgba_getb(b)) < 5 && rgba_geta(a) == rgba_geta(b))
          << " a=" << int(rgba_getr(a)) << "," << int(rgba_getg(a)) << "," << int(rgba_getb(a))
          << "," << int(rgba_geta(a)) << " b=" << int(rgba_getr(b)) << "," << int(rgba_getg(b))
          << "," << int(rgba_getb(b)) << "," << int(rgba_geta(b))
          << " blendMode=" << blend_mode_to_string(blendMode);
      }
    }
  }
}

#endif // SK_ENABLE_SKSL
