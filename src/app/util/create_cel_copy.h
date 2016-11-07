// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UTIL_CREATE_CEL_COPY_H_INCLUDED
#define APP_UTIL_CREATE_CEL_COPY_H_INCLUDED
#pragma once

#include "doc/frame.h"

namespace doc {
  class Cel;
  class Sprite;
}

namespace app {

  Cel* create_cel_copy(const Cel* srcCel,
                       const Sprite* dstSprite,
                       const Layer* dstLayer,
                       const frame_t dstFrame);

} // namespace app

#endif
