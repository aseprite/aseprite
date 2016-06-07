// Aseprite Document Library
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

// lay1 = A _ B
// lay2 = C D E
TEST(Sprite, CelsRange)
{
  Sprite* spr = new Sprite(IMAGE_RGB, 32, 32, 256);
  spr->setTotalFrames(3);

  LayerImage* lay1 = new LayerImage(spr);
  LayerImage* lay2 = new LayerImage(spr);
  spr->root()->addLayer(lay1);
  spr->root()->addLayer(lay2);

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

  int i = 0;
  for (Cel* cel : spr->cels()) {
    switch (i) {
      case 0: EXPECT_EQ(cel, celA); break;
      case 1: EXPECT_EQ(cel, celB); break;
      case 2: EXPECT_EQ(cel, celC); break;
      case 3: EXPECT_EQ(cel, celD); break;
      case 4: EXPECT_EQ(cel, celE); break;
    }
    ++i;
  }
  EXPECT_EQ(5, i);

  i = 0;
  for (Cel* cel : spr->uniqueCels()) {
    switch (i) {
      case 0: EXPECT_EQ(cel, celA); break;
      case 1: EXPECT_EQ(cel, celC); break;
      case 2: EXPECT_EQ(cel, celD); break;
    }
    ++i;
  }
  EXPECT_EQ(3, i);

  i = 0;
  for (Cel* cel : spr->cels(frame_t(0))) {
    switch (i) {
      case 0: EXPECT_EQ(cel, celA); break;
      case 1: EXPECT_EQ(cel, celC); break;
    }
    ++i;
  }
  EXPECT_EQ(2, i);

  i = 0;
  for (Cel* cel : spr->cels(frame_t(1))) {
    switch (i) {
      case 0: EXPECT_EQ(cel, celD); break;
    }
    ++i;
  }
  EXPECT_EQ(1, i);

  i = 0;
  for (Cel* cel : spr->cels(frame_t(2))) {
    switch (i) {
      case 0: EXPECT_EQ(cel, celB); break;
      case 1: EXPECT_EQ(cel, celE); break;
    }
    ++i;
  }
  EXPECT_EQ(2, i);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
