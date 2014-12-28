/* Aseprite
 * Copyright (C) 2001-2014  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "tests/test.h"

#include "app/document_api.h"
#include "app/test_context.h"
#include "base/unique_ptr.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/primitives.h"

using namespace app;
using namespace doc;

typedef base::UniquePtr<app::Document> DocumentPtr;

TEST(DocumentApi, MoveCel) {
  TestContext ctx;
  DocumentPtr doc(static_cast<app::Document*>(ctx.documents().add(32, 16)));
  Sprite* sprite = doc->sprite();
  LayerImage* layer1 = dynamic_cast<LayerImage*>(sprite->folder()->getFirstLayer());
  LayerImage* layer2 = new LayerImage(sprite);

  Cel* cel1 = layer1->getCel(frame_t(0));
  cel1->setPosition(2, -2);
  cel1->setOpacity(128);

  Image* image1 = cel1->image();
  EXPECT_EQ(32, image1->width());
  EXPECT_EQ(16, image1->height());
  for (int v=0; v<image1->height(); ++v)
    for (int u=0; u<image1->width(); ++u)
      image1->putPixel(u, v, u+v*image1->width());

  // Create a copy for later comparison.
  base::UniquePtr<Image> expectedImage(Image::createCopy(image1));

  doc->getApi().moveCel(
    layer1, frame_t(0),
    layer2, frame_t(1));

  EXPECT_EQ(NULL, layer1->getCel(frame_t(0)));

  Cel* cel2 = layer2->getCel(frame_t(1));
  ASSERT_TRUE(cel2 != NULL);

  Image* image2 = cel2->image();
  EXPECT_EQ(32, image2->width());
  EXPECT_EQ(16, image2->height());
  EXPECT_EQ(0, count_diff_between_images(expectedImage, image2));
  EXPECT_EQ(2, cel2->x());
  EXPECT_EQ(-2, cel2->y());
  EXPECT_EQ(128, cel2->opacity());

  doc->close();
}
