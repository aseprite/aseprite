/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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

#include "app/app.h"
#include "app/context.h"
#include "app/document.h"
#include "app/file/file.h"
#include "app/file/file_formats_manager.h"
#include "doc/doc.h"
#include "she/she.h"

#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace app;

TEST(File, SeveralSizes)
{
  she::ScopedHandle<she::System> system(she::create_system());
  // Register all possible image formats.
  FileFormatsManager::instance()->registerAllFormats();
  std::vector<char> fn(256);
  app::Context ctx;

  for (int w=10; w<=10+503*2; w+=503) {
    for (int h=10; h<=10+503*2; h+=503) {
      //std::sprintf(&fn[0], "test_%dx%d.ase", w, h);
      std::sprintf(&fn[0], "test.ase");

      {
        doc::Document* doc = ctx.documents().add(w, h, doc::ColorMode::INDEXED, 256);
        doc->setFilename(&fn[0]);

        // Random pixels
        LayerImage* layer = dynamic_cast<LayerImage*>(doc->sprite()->folder()->getFirstLayer());
        ASSERT_TRUE(layer != NULL);
        Image* image = layer->getCel(FrameNumber(0))->image();
        std::srand(w*h);
        int c = std::rand()%256;
        for (int y=0; y<h; y++) {
          for (int x=0; x<w; x++) {
            put_pixel_fast<IndexedTraits>(image, x, y, c);
            if ((std::rand()&4) == 0)
              c = std::rand()%256;
          }
        }

        save_document(&ctx, doc);
        doc->close();
        delete doc;
      }

      {
        app::Document* doc = load_document(&ctx, &fn[0]);
        ASSERT_EQ(w, doc->sprite()->width());
        ASSERT_EQ(h, doc->sprite()->height());

        // Same random pixels (see the seed)
        LayerImage* layer = dynamic_cast<LayerImage*>(doc->sprite()->folder()->getFirstLayer());
        ASSERT_TRUE(layer != NULL);
        Image* image = layer->getCel(FrameNumber(0))->image();
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
        delete doc;
      }
    }
  }
}
