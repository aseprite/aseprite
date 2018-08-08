// Aseprite Document Library
// Copyright (c) 2001-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/mask_io.h"

#include "base/serialization.h"
#include "doc/mask.h"

#include <iostream>
#include <memory>

namespace doc {

using namespace base::serialization;
using namespace base::serialization::little_endian;

// Serialized Mask data:
//
//   WORD[4]            x, y, w, h
//   for each line      ("h" times)
//     for each packet  ("((w+7)/8)" times)
//       BYTE           8 pixels of the mask
//       BYTE           for Indexed images

void write_mask(std::ostream& os, const Mask* mask)
{
  const gfx::Rect& bounds = mask->bounds();

  write16(os, bounds.x);                        // Xpos
  write16(os, bounds.y);                        // Ypos
  write16(os, mask->bitmap() ? bounds.w: 0);    // Width
  write16(os, mask->bitmap() ? bounds.h: 0);    // Height

  if (mask->bitmap()) {
    int size = BitmapTraits::getRowStrideBytes(bounds.w);

    for (int c=0; c<bounds.h; c++)
      os.write((char*)mask->bitmap()->getPixelAddress(0, c), size);
  }
}

Mask* read_mask(std::istream& is)
{
  int x = int16_t(read16(is));  // Xpos (it's a signed int16 because we support negative mask coordinates)
  int y = int16_t(read16(is));  // Ypos
  int w = read16(is);           // Width
  int h = read16(is);           // Height

  std::unique_ptr<Mask> mask(new Mask());

  if (w > 0 && h > 0) {
    int size = BitmapTraits::getRowStrideBytes(w);

    mask->add(gfx::Rect(x, y, w, h));
    for (int c=0; c<mask->bounds().h; c++)
      is.read((char*)mask->bitmap()->getPixelAddress(0, c), size);
  }

  return mask.release();
}

}
