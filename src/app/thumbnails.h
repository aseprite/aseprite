// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2016  Carlo Caputo
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_THUMBNAILS_H_INCLUDED
#define APP_THUMBNAILS_H_INCLUDED
#pragma once

#include "gfx/size.h"
#include "os/surface.h"

namespace doc {
class Cel;
}

namespace os {
class Surface;
}

namespace app { namespace thumb {

os::SurfaceRef get_cel_thumbnail(const doc::Cel* cel, const gfx::Size& fitInSize);

}} // namespace app::thumb

#endif
