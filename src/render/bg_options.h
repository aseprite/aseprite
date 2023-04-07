// Aseprite Render Library
// Copyright (c) 2022  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef RENDER_BG_OPTIONS_H_INCLUDED
#define RENDER_BG_OPTIONS_H_INCLUDED
#pragma once

#include "doc/color.h"
#include "doc/frame.h"
#include "gfx/size.h"
#include "render/bg_type.h"

namespace render {

  struct BgOptions {
    BgType type = BgType::TRANSPARENT;
    bool zoom = false;
    doc::PixelFormat colorPixelFormat = doc::PixelFormat::IMAGE_RGB;
    doc::color_t color1 = 0;
    doc::color_t color2 = 0;
    gfx::Size stripeSize{16, 16};

    BgOptions(BgType type = BgType::TRANSPARENT) : type(type) { }

    static BgOptions MakeNone() { return BgOptions(BgType::NONE); }
    static BgOptions MakeTransparent() { return BgOptions(BgType::TRANSPARENT); }
  };

} // namespace render

#endif
