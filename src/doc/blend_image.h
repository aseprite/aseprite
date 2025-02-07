// Aseprite Document Library
// Copyright (C) 2019-2024  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_BLEND_IMAGE_H_INCLUDED
#define DOC_BLEND_IMAGE_H_INCLUDED
#pragma once

#include "doc/blend_mode.h"
#include "gfx/fwd.h"

namespace doc {
class Image;
class Palette;

void blend_image(Image* dst,
                 const Image* src,
                 gfx::Clip area,
                 // For indexed color mode
                 const Palette* pal,
                 // For grayscale/RGB color modes
                 const int opacity,
                 const doc::BlendMode blendMode);

} // namespace doc

#endif
