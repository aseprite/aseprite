// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "tests/test.h"

#include "app/context.h"
#include "app/doc.h"
#include "app/doc_api.h"
#include "app/test_context.h"
#include "app/tx.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/primitives.h"

using namespace app;
using namespace doc;

typedef std::unique_ptr<Doc> DocPtr;

class BasicDocApiTest : public ::testing::Test {
public:
  BasicDocApiTest() :
    doc((ctx.documents().add(32, 16))),
    sprite(doc->sprite()),
    root(sprite->root()),
    layer1(dynamic_cast<LayerImage*>(sprite->root()->firstLayer())),
    layer2(new LayerImage(sprite)),
    layer3(new LayerImage(sprite))
  {
    root->addLayer(layer2);
    root->addLayer(layer3);
  }

  ~BasicDocApiTest() {
    doc->close();
  }

  TestContextT<Context> ctx;
  DocPtr doc;
  Sprite* sprite;
  LayerGroup* root;
  LayerImage* layer1;
  LayerImage* layer2;
  LayerImage* layer3;
};

TEST_F(BasicDocApiTest, RestackLayerBefore)
{
  EXPECT_EQ(layer1, root->firstLayer());
  {
    Tx tx(&ctx, "");
    // Do nothing
    doc->getApi(tx).restackLayerBefore(layer1, layer1->parent(), layer1);
    EXPECT_EQ(layer1, root->firstLayer());
    EXPECT_EQ(layer2, root->firstLayer()->getNext());
    EXPECT_EQ(layer3, root->firstLayer()->getNext()->getNext());
    // Rollback
  }

  EXPECT_EQ(layer1, root->firstLayer());
  {
    Tx tx(&ctx, "");
    doc->getApi(tx).restackLayerBefore(layer1, layer3->parent(), layer3);
    EXPECT_EQ(layer2, root->firstLayer());
    EXPECT_EQ(layer1, root->firstLayer()->getNext());
    EXPECT_EQ(layer3, root->firstLayer()->getNext()->getNext());
    // Rollback
  }

  EXPECT_EQ(layer1, root->firstLayer());
  {
    Tx tx(&ctx, "");
    doc->getApi(tx).restackLayerBefore(layer1, layer1->parent(), nullptr);
    EXPECT_EQ(layer2, root->firstLayer());
    EXPECT_EQ(layer3, root->firstLayer()->getNext());
    EXPECT_EQ(layer1, root->firstLayer()->getNext()->getNext());
    // Rollback
  }
}

TEST_F(BasicDocApiTest, RestackLayerAfter)
{
  EXPECT_EQ(layer1, root->firstLayer());
  {
    Tx tx(&ctx, "");
    // Do nothing
    doc->getApi(tx).restackLayerAfter(layer1, layer1->parent(), layer1);
    EXPECT_EQ(layer1, root->firstLayer());
    EXPECT_EQ(layer2, root->firstLayer()->getNext());
    EXPECT_EQ(layer3, root->firstLayer()->getNext()->getNext());
    // Rollback
  }

  EXPECT_EQ(layer1, root->firstLayer());
  {
    Tx tx(&ctx, "");
    doc->getApi(tx).restackLayerAfter(layer1, layer3->parent(), layer3);
    EXPECT_EQ(layer2, root->firstLayer());
    EXPECT_EQ(layer3, root->firstLayer()->getNext());
    EXPECT_EQ(layer1, root->firstLayer()->getNext()->getNext());
    // Rollback
  }

  EXPECT_EQ(layer1, root->firstLayer());
  {
    Tx tx(&ctx, "");
    doc->getApi(tx).restackLayerAfter(layer3, layer3->parent(), nullptr);
    EXPECT_EQ(layer3, root->firstLayer());
    EXPECT_EQ(layer1, root->firstLayer()->getNext());
    EXPECT_EQ(layer2, root->firstLayer()->getNext()->getNext());
    // Rollback
  }
}

TEST_F(BasicDocApiTest, MoveCel)
{
  Cel* cel1 = layer1->cel(frame_t(0));
  cel1->setPosition(2, -2);
  cel1->setOpacity(128);

  Image* image1 = cel1->image();
  EXPECT_EQ(32, image1->width());
  EXPECT_EQ(16, image1->height());
  for (int v=0; v<image1->height(); ++v)
    for (int u=0; u<image1->width(); ++u)
      image1->putPixel(u, v, u+v*image1->width());

  // Create a copy for later comparison.
  std::unique_ptr<Image> expectedImage(Image::createCopy(image1));

  Tx tx(&ctx, "");
  doc->getApi(tx).moveCel(
    layer1, frame_t(0),
    layer2, frame_t(1));
  tx.commit();

  EXPECT_EQ(NULL, layer1->cel(frame_t(0)));

  Cel* cel2 = layer2->cel(frame_t(1));
  ASSERT_TRUE(cel2 != NULL);

  Image* image2 = cel2->image();
  EXPECT_EQ(32, image2->width());
  EXPECT_EQ(16, image2->height());
  EXPECT_EQ(0, count_diff_between_images(expectedImage.get(), image2));
  EXPECT_EQ(2, cel2->x());
  EXPECT_EQ(-2, cel2->y());
  EXPECT_EQ(128, cel2->opacity());
}
