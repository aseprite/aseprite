// Aseprite
// Copyright (C) 2018-2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "tests/test.h"

#include "app/app.h"
#include "app/context.h"
#include "app/doc.h"
#include "app/file/file.h"
#include "app/file/file_formats_manager.h"
#include "doc/doc.h"

#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace app;

TEST(File, SeveralSizes)
{
  // Register all possible image formats.
  std::vector<char> fn(256);
  app::Context ctx;

  for (int w=10; w<=10+503*2; w+=503) {
    for (int h=10; h<=10+503*2; h+=503) {
      //std::sprintf(&fn[0], "test_%dx%d.ase", w, h);
      std::sprintf(&fn[0], "test.ase");

      {
        std::unique_ptr<Doc> doc(
          ctx.documents().add(w, h, doc::ColorMode::INDEXED, 256));
        doc->setFilename(&fn[0]);

        // Random pixels
        Layer* layer = doc->sprite()->root()->firstLayer();
        ASSERT_TRUE(layer != NULL);
        Image* image = layer->cel(frame_t(0))->image();
        std::srand(w*h);
        int c = std::rand()%256;
        for (int y=0; y<h; y++) {
          for (int x=0; x<w; x++) {
            put_pixel_fast<IndexedTraits>(image, x, y, c);
            if ((std::rand()&4) == 0)
              c = std::rand()%256;
          }
        }

        save_document(&ctx, doc.get());
        doc->close();
      }

      {
        std::unique_ptr<Doc> doc(load_document(&ctx, &fn[0]));
        ASSERT_EQ(w, doc->sprite()->width());
        ASSERT_EQ(h, doc->sprite()->height());

        // Same random pixels (see the seed)
        Layer* layer = doc->sprite()->root()->firstLayer();
        ASSERT_TRUE(layer != nullptr);
        Image* image = layer->cel(frame_t(0))->image();
        std::srand(w*h);
        int c = std::rand()%256;
        for (int y=0; y<h; y++) {
          for (int x=0; x<w; x++) {
            ASSERT_EQ(c, get_pixel_fast<IndexedTraits>(image, x, y));
            if ((std::rand()&4) == 0)
              c = std::rand()%256;
          }
        }

        doc->close();
      }
    }
  }
}
