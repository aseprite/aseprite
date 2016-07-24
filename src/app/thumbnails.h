#ifndef APP_THUMBNAILS_H_INCLUDED
#define APP_THUMBNAILS_H_INCLUDED
#pragma once

#include "gfx/rect.h"

namespace doc {
  class Cel;
}

namespace she {
  class Surface;
}

namespace app {
  namespace thumb {

    she::Surface* get_surface(const doc::Cel* cel, const gfx::Rect& bounds);

  } // thumb
} // app

#endif