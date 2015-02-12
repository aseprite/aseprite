// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_CMD_WITH_IMAGE_H_INCLUDED
#define APP_CMD_WITH_IMAGE_H_INCLUDED
#pragma once

#include "doc/object_id.h"

namespace doc {
  class Image;
}

namespace app {
namespace cmd {
  using namespace doc;

  class WithImage {
  public:
    WithImage(Image* image);
    Image* image();

  private:
    ObjectId m_imageId;
  };

} // namespace cmd
} // namespace app

#endif
