// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/util/pic_file.h"
#include "base/cfile.h"
#include "base/file_handle.h"
#include "base/fs.h"
#include "doc/image.h"
#include "doc/mask.h"

#include <memory>

namespace app {

using namespace doc;

// Loads a MSK file (Animator and Animator Pro format)
Mask* load_msk_file(const char* filename)
{
  int orig_size = base::file_size(filename);
  int i, c, u, v, byte, magic, size;
  Mask* mask = NULL;

  FILE* f = base::open_file_raw(filename, "r");
  if (!f)
    return NULL;

  size = base::fgetl(f);
  magic = base::fgetw(f);

  // Animator Pro MSK format
  if ((size == orig_size) && (magic == 0x9500)) {
    fclose(f);

    // Just load an Animator Pro PIC file
    int x, y;
    std::unique_ptr<Image> image(load_pic_file(filename, &x, &y, NULL));

    if (image != NULL && (image->pixelFormat() == IMAGE_BITMAP)) {
      mask = new Mask();
      mask->replace(gfx::Rect(x, y, image->width(), image->height()));
      mask->bitmap()->copy(image.get(), gfx::Clip(image->bounds()));
      mask->shrink();
    }
  }
  // Animator MSK format
  else if (orig_size == 8000) {
    mask = new Mask();
    mask->replace(gfx::Rect(0, 0, 320, 200));

    u = v = 0;
    for (i=0; i<8000; i++) {
      byte = getc(f);
      for (c=0; c<8; c++) {
        mask->bitmap()->putPixel(u, v, byte & (1<<(7-c)));
        u++;
        if (u == 320) {
          u = 0;
          v++;
        }
      }
    }
    fclose(f);
  }
  else {
    fclose(f);
  }

  return mask;
}

// Saves an Animator Pro MSK file (really a PIC file)
int save_msk_file(const Mask* mask, const char* filename)
{
  if (mask->bitmap())
    return save_pic_file(filename,
                         mask->bounds().x,
                         mask->bounds().y, NULL,
                         mask->bitmap());
  else
    return -1;
}

} // namespace app
