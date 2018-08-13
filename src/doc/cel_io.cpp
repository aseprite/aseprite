// Aseprite Document Library
// Copyright (c) 2001-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/cel_io.h"

#include "base/serialization.h"
#include "doc/cel.h"
#include "doc/subobjects_io.h"

#include <iostream>
#include <memory>

namespace doc {

using namespace base::serialization;
using namespace base::serialization::little_endian;

void write_cel(std::ostream& os, const Cel* cel)
{
  write32(os, cel->id());
  write16(os, cel->frame());
  write32(os, cel->dataRef()->id());
}

Cel* read_cel(std::istream& is, SubObjectsIO* subObjects, bool setId)
{
  ObjectId id = read32(is);
  frame_t frame(read16(is));
  ObjectId celDataId = read32(is);
  CelDataRef celData(subObjects->getCelDataRef(celDataId));
  if (!celData)
    return nullptr;

  std::unique_ptr<Cel> cel(new Cel(frame, celData));
  if (setId)
    cel->setId(id);
  return cel.release();
}

}
