// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/cel_io.h"

#include "base/serialization.h"
#include "base/unique_ptr.h"
#include "doc/cel.h"
#include "doc/sprite.h"
#include "doc/subobjects_io.h"

#include <iostream>

namespace doc {

using namespace base::serialization;
using namespace base::serialization::little_endian;

void write_cel(std::ostream& os, SubObjectsIO* subObjects, Cel* cel)
{
  Cel* link = cel->link();

  write16(os, cel->frame());
  write16(os, (int16_t)cel->x());
  write16(os, (int16_t)cel->y());
  write16(os, cel->opacity());
  write16(os, link ? 1: 0);
  if (link)
    write32(os, link->id());
  else
    subObjects->write_image(os, cel->image());
}

Cel* read_cel(std::istream& is, SubObjectsIO* subObjects, Sprite* sprite)
{
  frame_t frame(read16(is));
  int x = (int16_t)read16(is);
  int y = (int16_t)read16(is);
  int opacity = read16(is);
  bool is_link = (read16(is) == 1);

  base::UniquePtr<Cel> cel(new Cel(frame, ImageRef(NULL)));

  cel->setPosition(x, y);
  cel->setOpacity(opacity);

  if (is_link) {
    ObjectId imageId = read32(is);
    cel->setImage(sprite->getImage(imageId));
  }
  else
    cel->setImage(ImageRef(subObjects->read_image(is)));

  return cel.release();
}

}
