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

#include <iostream>

namespace doc {

using namespace base::serialization;
using namespace base::serialization::little_endian;

// Serialized Cel data:
//
//   WORD               Frame
//   WORD               Image index
//   WORD[2]            X, Y
//   WORD               Opacity

void write_cel(std::ostream& os, Cel* cel)
{
  // ObjectId cel_id = objects->addObject(cel);
  // write_raw_uint32(cel_id);

  write16(os, cel->frame());
  write16(os, cel->imageIndex());
  write16(os, (int16_t)cel->x());
  write16(os, (int16_t)cel->y());
  write16(os, cel->opacity());

  // objects->removeObject(cel_id);
}

Cel* read_cel(std::istream& is)
{
  // ObjectId cel_id = read32();

  FrameNumber frame(read16(is));
  int imageIndex = read16(is);
  int x = (int16_t)read16(is);
  int y = (int16_t)read16(is);
  int opacity = read16(is);

  base::UniquePtr<Cel> cel(new Cel(frame, imageIndex));

  cel->setPosition(x, y);
  cel->setOpacity(opacity);

  // objects->insertObject(cel_id, cel);
  return cel.release();
}

}
