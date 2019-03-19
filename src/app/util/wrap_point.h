// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_WRAP_POINT_H_INCLUDED
#define APP_WRAP_POINT_H_INCLUDED
#pragma once

#include "filters/tiled_mode.h"
#include "gfx/point.h"
#include "gfx/size.h"

namespace app {

  gfx::Point wrap_point(const filters::TiledMode tiledMode,
                        const gfx::Size& spriteSize,
                        const gfx::Point& pt,
                        const bool clamp);

  gfx::PointF wrap_pointF(const filters::TiledMode tiledMode,
                          const gfx::Size& spriteSize,
                          const gfx::PointF& pt);

} // namespace app

#endif
