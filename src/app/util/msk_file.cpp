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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <allegro.h>

#include "app/util/pic_file.h"
#include "base/unique_ptr.h"
#include "raster/image.h"
#include "raster/mask.h"

namespace app {

using namespace raster;

// Loads a MSK file (Animator and Animator Pro format)
Mask* load_msk_file(const char* filename)
{
#if (MAKE_VERSION(4, 2, 1) >= MAKE_VERSION(ALLEGRO_VERSION,             \
                                           ALLEGRO_SUB_VERSION,         \
                                           ALLEGRO_WIP_VERSION))
  int orig_size = file_size(filename);
#else
  int orig_size = file_size_ex(filename);
#endif
  int i, c, u, v, byte, magic, size;
  Mask *mask = NULL;
  PACKFILE *f;

  f = pack_fopen(filename, F_READ);
  if (!f)
    return NULL;

  size = pack_igetl(f);
  magic = pack_igetw(f);

  // Animator Pro MSK format
  if ((size == orig_size) && (magic == 0x9500)) {
    int x, y;

    pack_fclose(f);

    // Just load an Animator Pro PIC file
    base::UniquePtr<Image> image(load_pic_file(filename, &x, &y, NULL));
    if (image != NULL && (image->getPixelFormat() == IMAGE_BITMAP)) {
      mask = new Mask(x, y, image.release());
    }
  }
  // Animator MSK format
  else if (orig_size == 8000) {
    mask = new Mask();
    mask->replace(0, 0, 320, 200);

    u = v = 0;
    for (i=0; i<8000; i++) {
      byte = pack_getc (f);
      for (c=0; c<8; c++) {
        mask->getBitmap()->putpixel(u, v, byte & (1<<(7-c)));
        u++;
        if (u == 320) {
          u = 0;
          v++;
        }
      }
    }
    pack_fclose(f);
  }
  else {
    pack_fclose(f);
  }

  return mask;
}

// Saves an Animator Pro MSK file (really a PIC file)
int save_msk_file(const Mask* mask, const char* filename)
{
  if (mask->getBitmap())
    return save_pic_file(filename,
                         mask->getBounds().x,
                         mask->getBounds().y, NULL,
                         mask->getBitmap());
  else
    return -1;
}

} // namespace app
