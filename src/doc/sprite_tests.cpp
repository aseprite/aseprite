// Aseprite Document Library
// Copyright (c) 2018 Igara Studio S.A.
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtest/gtest.h>

#include "doc/cel.h"
#include "doc/cels_range.h"
#include "doc/layer.h"
#include "doc/pixel_format.h"
#include "doc/sprite.h"

using namespace doc;

TEST(Sprite, Layers)
{
  Sprite* spr = new Sprite(ImageSpec(ColorMode::RGB, 32, 32), 256);

  LayerImage* lay1 = new LayerImage(spr);
  LayerImage* lay2 = new LayerImage(spr);
  LayerImage* lay3 = new LayerImage(spr);
  LayerImage* lay4 = new LayerImage(spr);
  LayerImage* lay5 = new LayerImage(spr);
  LayerImage* lay6 = new LayerImage(spr);
  LayerImage* lay7 = new LayerImage(spr);
  LayerGroup* grp1 = new LayerGroup(spr);
  LayerGroup* grp2 = new LayerGroup(spr);
  LayerGroup* grp3 = new LayerGroup(spr);

  grp1->setVisible(false);
  lay5->setVisible(false);
  grp2->setCollapsed(true);

  ;; grp2->addLayer(lay5);
  ;;;; grp3->addLayer(lay7);
  ;;;; grp3->addLayer(lay6);
  ;; grp2->addLayer(grp3);
  spr->root()->addLayer(grp2);
  spr->root()->addLayer(lay2);
  ;; grp1->addLayer(lay4);
  ;; grp1->addLayer(lay3);
  spr->root()->addLayer(grp1);
  spr->root()->addLayer(lay1);

  auto all = spr->allLayers();
  ASSERT_EQ(10, all.size());
  EXPECT_EQ(lay5, all[0]);
  EXPECT_EQ(lay7, all[1]);
  EXPECT_EQ(lay6, all[2]);
  EXPECT_EQ(grp3, all[3]);
  EXPECT_EQ(grp2, all[4]);
  EXPECT_EQ(lay2, all[5]);
  EXPECT_EQ(lay4, all[6]);
  EXPECT_EQ(lay3, all[7]);
  EXPECT_EQ(grp1, all[8]);
  EXPECT_EQ(lay1, all[9]);

  auto vis = spr->allVisibleLayers();
  ASSERT_EQ(6, vis.size());
  EXPECT_EQ(lay7, vis[0]);
  EXPECT_EQ(lay6, vis[1]);
  EXPECT_EQ(grp3, vis[2]);
  EXPECT_EQ(grp2, vis[3]);
  EXPECT_EQ(lay2, vis[4]);
  EXPECT_EQ(lay1, vis[5]);

  auto bro = spr->allBrowsableLayers();
  ASSERT_EQ(6, bro.size());
  EXPECT_EQ(grp2, bro[0]);
  EXPECT_EQ(lay2, bro[1]);
  EXPECT_EQ(lay4, bro[2]);
  EXPECT_EQ(lay3, bro[3]);
  EXPECT_EQ(grp1, bro[4]);
  EXPECT_EQ(lay1, bro[5]);
}

//            frames
//            0 1 2
// root
// - lay1:    A~~~B
// - lay2:    C D~E
// - grp1:
//   - lay3:  F G~H
TEST(Sprite, CelsRange)
{
  Sprite* spr = new Sprite(ImageSpec(ColorMode::RGB, 32, 32), 256);
  spr->setTotalFrames(3);

  LayerImage* lay1 = new LayerImage(spr);
  LayerImage* lay2 = new LayerImage(spr);
  LayerGroup* grp1 = new LayerGroup(spr);
  LayerImage* lay3 = new LayerImage(spr);
  spr->root()->addLayer(lay1);
  spr->root()->addLayer(lay2);
  spr->root()->addLayer(grp1);
  grp1->addLayer(lay3);

  ImageRef imgA(Image::create(IMAGE_RGB, 32, 32));
  Cel* celA = new Cel(frame_t(0), imgA);
  Cel* celB = Cel::createLink(celA);
  celB->setFrame(frame_t(2));
  lay1->addCel(celA);
  lay1->addCel(celB);

  ImageRef imgC(Image::create(IMAGE_RGB, 32, 32));
  Cel* celC = new Cel(frame_t(0), imgC);
  Cel* celD = Cel::createCopy(celC);
  Cel* celE = Cel::createLink(celD);
  celD->setFrame(frame_t(1));
  celE->setFrame(frame_t(2));
  lay2->addCel(celC);
  lay2->addCel(celD);
  lay2->addCel(celE);

  ImageRef imgF(Image::create(IMAGE_RGB, 32, 32));
  ImageRef imgG(Image::create(IMAGE_RGB, 32, 32));
  Cel* celF = new Cel(frame_t(0), imgF);
  Cel* celG = new Cel(frame_t(1), imgG);
  Cel* celH = Cel::createLink(celG);
  celH->setFrame(frame_t(2));
  lay3->addCel(celF);
  lay3->addCel(celG);
  lay3->addCel(celH);

  int i = 0;
  for (Cel* cel : spr->cels()) {
    switch (i) {
      case 0: EXPECT_EQ(cel, celA); break;
      case 1: EXPECT_EQ(cel, celB); break;
      case 2: EXPECT_EQ(cel, celC); break;
      case 3: EXPECT_EQ(cel, celD); break;
      case 4: EXPECT_EQ(cel, celE); break;
      case 5: EXPECT_EQ(cel, celF); break;
      case 6: EXPECT_EQ(cel, celG); break;
      case 7: EXPECT_EQ(cel, celH); break;
    }
    ++i;
  }
  EXPECT_EQ(8, i);

  i = 0;
  for (Cel* cel : spr->uniqueCels()) {
    switch (i) {
      case 0: EXPECT_EQ(cel, celA); break;
      case 1: EXPECT_EQ(cel, celC); break;
      case 2: EXPECT_EQ(cel, celD); break;
      case 3: EXPECT_EQ(cel, celF); break;
      case 4: EXPECT_EQ(cel, celG); break;
    }
    ++i;
  }
  EXPECT_EQ(5, i);

  i = 0;
  for (Cel* cel : spr->cels(frame_t(0))) {
    switch (i) {
      case 0: EXPECT_EQ(cel, celA); break;
      case 1: EXPECT_EQ(cel, celC); break;
      case 2: EXPECT_EQ(cel, celF); break;
    }
    ++i;
  }
  EXPECT_EQ(3, i);

  i = 0;
  for (Cel* cel : spr->cels(frame_t(1))) {
    switch (i) {
      case 0: EXPECT_EQ(cel, celD); break;
      case 1: EXPECT_EQ(cel, celG); break;
    }
    ++i;
  }
  EXPECT_EQ(2, i);

  i = 0;
  for (Cel* cel : spr->cels(frame_t(2))) {
    switch (i) {
      case 0: EXPECT_EQ(cel, celB); break;
      case 1: EXPECT_EQ(cel, celE); break;
      case 2: EXPECT_EQ(cel, celH); break;
    }
    ++i;
  }
  EXPECT_EQ(3, i);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
