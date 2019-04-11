// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2016  Carlo Caputo
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_THUMBNAILS_H_INCLUDED
#define APP_THUMBNAILS_H_INCLUDED
#pragma once

#include "gfx/size.h"

namespace doc {
  class Cel;
}

namespace os {
  class Surface;
}

namespace app {
namespace thumb {

  os::Surface* get_cel_thumbnail(const doc::Cel* cel,
                                 const gfx::Size& fitInSize);

} // thumb
} // app

#endif
