/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#include "config.h"

#include "raster/mask_io.h"

#include "base/serialization.h"
#include "base/unique_ptr.h"
#include "raster/mask.h"

#include <iostream>

namespace raster {

using namespace base::serialization;
using namespace base::serialization::little_endian;

// Serialized Mask data:
//
//   WORD[4]            x, y, w, h
//   for each line      ("h" times)
//     for each packet  ("((w+7)/8)" times)
//       BYTE           8 pixels of the mask
//       BYTE           for Indexed images

void write_mask(std::ostream& os, Mask* mask)
{
  write16(os, mask->x);                // Xpos
  write16(os, mask->y);                // Ypos
  write16(os, mask->bitmap ? mask->w: 0); // Width
  write16(os, mask->bitmap ? mask->h: 0); // Height

  if (mask->bitmap) {
    int size = (mask->w+7)/8;

    for (int c=0; c<mask->h; c++)
      os.write((char*)mask->bitmap->line[c], size);
  }
}

Mask* read_mask(std::istream& is)
{
  int x = read16(is);           // Xpos
  int y = read16(is);           // Ypos
  int w = read16(is);           // Width
  int h = read16(is);           // Height

  UniquePtr<Mask> mask(mask_new());

  if (w > 0 && h > 0) {
    int size = (w+7)/8;

    mask->add(x, y, w, h);
    for (int c=0; c<mask->h; c++)
      is.read((char*)mask->bitmap->line[c], size);
  }

  return mask.release();
}

}
