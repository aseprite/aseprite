// Aseprite Document Library
// Copyright (c) 2001-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/cel_data_io.h"

#include "base/serialization.h"
#include "doc/cel_data.h"
#include "doc/subobjects_io.h"
#include "doc/user_data_io.h"

#include <iostream>
#include <memory>

namespace doc {

using namespace base::serialization;
using namespace base::serialization::little_endian;

void write_celdata(std::ostream& os, const CelData* celdata)
{
  write32(os, celdata->id());
  write32(os, celdata->bounds().x);
  write32(os, celdata->bounds().y);
  write32(os, celdata->bounds().w);
  write32(os, celdata->bounds().h);
  write8(os, celdata->opacity());
  write32(os, celdata->image()->id());
  write_user_data(os, celdata->userData());
}

CelData* read_celdata(std::istream& is, SubObjectsIO* subObjects, bool setId)
{
  ObjectId id = read32(is);
  int x = read32(is);
  int y = read32(is);
  int w = read32(is);
  int h = read32(is);
  int opacity = read8(is);
  ObjectId imageId = read32(is);
  UserData userData = read_user_data(is);

  ImageRef image(subObjects->getImageRef(imageId));
  if (!image)
    return nullptr;

  std::unique_ptr<CelData> celdata(new CelData(image));
  celdata->setBounds(gfx::Rect(x, y, w, h));
  celdata->setOpacity(opacity);
  celdata->setUserData(userData);
  if (setId)
    celdata->setId(id);
  return celdata.release();
}

}
