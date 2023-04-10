// Aseprite Document Library
// Copyright (c) 2023 Igara Studio S.A.
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
  write16(os, uint16_t(int16_t(cel->zIndex())));
}

Cel* read_cel(std::istream& is, SubObjectsIO* subObjects, bool setId)
{
  ObjectId id = read32(is);
  frame_t frame(read16(is));
  ObjectId celDataId = read32(is);
  int zIndex = int(int16_t(read16(is)));
  if (is.eof())
    zIndex = 0;

  CelDataRef celData(subObjects->getCelDataRef(celDataId));
  if (!celData)
    return nullptr;

  auto cel = std::make_unique<Cel>(frame, celData);
  cel->setZIndex(zIndex);
  if (setId)
    cel->setId(id);
  return cel.release();
}

}
