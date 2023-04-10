// Aseprite Document Library
// Copyright (c) 2023  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtest/gtest.h>

#include "doc/render_plan.h"

#include "doc/cel.h"
#include "doc/document.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/sprite.h"

#include <memory>

using namespace doc;

#define HELPER_LOG(a, b) \
  a->layer()->name() << " instead of " << b->layer()->name()

#define EXPECT_PLAN(a, b, c, d)                                         \
  {                                                                     \
    RenderPlan plan;                                                    \
    plan.addLayer(spr->root(), 0);                                      \
    const auto& cels = plan.cels();                                     \
    EXPECT_EQ(a, cels[0]) << HELPER_LOG(cels[0], a);                    \
    EXPECT_EQ(b, cels[1]) << HELPER_LOG(cels[1], b);                    \
    EXPECT_EQ(c, cels[2]) << HELPER_LOG(cels[2], c);                    \
    EXPECT_EQ(d, cels[3]) << HELPER_LOG(cels[3], d);                    \
  }

TEST(RenderPlan, ZIndex)
{
  auto doc = std::make_shared<Document>();
  ImageSpec spec(ColorMode::INDEXED, 2, 2);
  Sprite* spr;
  doc->sprites().add(spr = Sprite::MakeStdSprite(spec));

  LayerImage
    *lay0 = static_cast<LayerImage*>(spr->root()->firstLayer()),
    *lay1 = new LayerImage(spr),
    *lay2 = new LayerImage(spr),
    *lay3 = new LayerImage(spr);

  lay0->setName("a");
  lay1->setName("b");
  lay2->setName("c");
  lay3->setName("d");

  Cel* a = lay0->cel(0),
    *b, *c, *d;
  lay1->addCel(b = new Cel(0, ImageRef(Image::create(spec))));
  lay2->addCel(c = new Cel(0, ImageRef(Image::create(spec))));
  lay3->addCel(d = new Cel(0, ImageRef(Image::create(spec))));

  spr->root()->insertLayer(lay1, lay0);
  spr->root()->insertLayer(lay2, lay1);
  spr->root()->insertLayer(lay3, lay2);

  a->setZIndex(0); EXPECT_PLAN(a, b, c, d);
  a->setZIndex(1); EXPECT_PLAN(b, a, c, d);
  a->setZIndex(2); EXPECT_PLAN(b, c, a, d);
  a->setZIndex(3); EXPECT_PLAN(b, c, d, a);
  a->setZIndex(4); EXPECT_PLAN(b, c, d, a);
  a->setZIndex(1000); EXPECT_PLAN(b, c, d, a);
  a->setZIndex(0); EXPECT_PLAN(a, b, c, d); // Back no normal

  b->setZIndex(-1); EXPECT_PLAN(b, a, c, d);
  b->setZIndex(-2); EXPECT_PLAN(b, a, c, d);
  b->setZIndex(-3); EXPECT_PLAN(b, a, c, d);
  b->setZIndex(-1000); EXPECT_PLAN(b, a, c, d);
  b->setZIndex(0); EXPECT_PLAN(a, b, c, d); // Back no normal

  a->setZIndex(-1);
  b->setZIndex(-1);
  c->setZIndex(-1);
  d->setZIndex(-1);
  EXPECT_PLAN(a, b, c, d);

  a->setZIndex(2);
  b->setZIndex(-1);
  c->setZIndex(0);
  d->setZIndex(-1);
  EXPECT_PLAN(b, d, c, a);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
