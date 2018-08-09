// Aseprite
// Copyright (C) 2016  Carlo Caputo
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_THUMBNAILS_H_INCLUDED
#define APP_THUMBNAILS_H_INCLUDED
#pragma once

#include "gfx/rect.h"
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
                                    const gfx::Size& thumb_size,
                                    gfx::Rect cel_image_on_thumb = gfx::Rect());

  } // thumb
} // app

#endif
