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
  write16(os, cel->frame());
  write16(os, (int16_t)cel->x());
  write16(os, (int16_t)cel->y());
  write8(os, cel->opacity());
  write32(os, cel->image()->id());
}

Cel* read_cel(std::istream& is, SubObjectsIO* subObjects, Sprite* sprite)
{
  frame_t frame(read16(is));
  int x = (int16_t)read16(is);
  int y = (int16_t)read16(is);
  int opacity = read8(is);

  base::UniquePtr<Cel> cel(new Cel(frame, ImageRef(NULL)));
  cel->setPosition(x, y);
  cel->setOpacity(opacity);

  ObjectId imageId = read32(is);
  cel->setImage(subObjects->get_image_ref(imageId));

  return cel.release();
}

}
