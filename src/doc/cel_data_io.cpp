// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/cel_data_io.h"

#include "base/serialization.h"
#include "base/unique_ptr.h"
#include "doc/cel_data.h"
#include "doc/subobjects_io.h"

#include <iostream>

namespace doc {

using namespace base::serialization;
using namespace base::serialization::little_endian;

void write_celdata(std::ostream& os, const CelData* celdata)
{
  write32(os, celdata->id());
  write16(os, (int16_t)celdata->position().x);
  write16(os, (int16_t)celdata->position().y);
  write8(os, celdata->opacity());
  write32(os, celdata->image()->id());
}

CelData* read_celdata(std::istream& is, SubObjectsIO* subObjects)
{
  ObjectId id = read32(is);
  int x = (int16_t)read16(is);
  int y = (int16_t)read16(is);
  int opacity = read8(is);

  ObjectId imageId = read32(is);
  ImageRef image(subObjects->getImageRef(imageId));

  base::UniquePtr<CelData> celdata(new CelData(image));
  celdata->setPosition(x, y);
  celdata->setOpacity(opacity);
  celdata->setId(id);
  return celdata.release();
}

}
