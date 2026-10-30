// Aseprite Document Library
// Copyright (C) 2019-present  Igara Studio S.A.
// Copyright (c) 2001-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_BLEND_FUNCS_H_INCLUDED
#define DOC_BLEND_FUNCS_H_INCLUDED
#pragma once

#include "doc/blend_mode.h"
#include "doc/color.h"
#include "doc/color_mode.h"

namespace doc {

using BlendFunc = color_t (*)(color_t backdrop, color_t src, int opacity);

color_t rgba_blender_merge(color_t backdrop, color_t src, int opacity);
color_t rgba_blender_neg_bw(color_t backdrop, color_t src, int opacity);
color_t rgba_blender_normal(color_t backdrop, color_t src, int opacity = 255);

color_t graya_blender_merge(color_t backdrop, color_t src, int opacity);
color_t graya_blender_neg_bw(color_t backdrop, color_t src, int opacity);
color_t graya_blender_normal(color_t backdrop, color_t src, int opacity = 255);

BlendFunc get_rgba_blender(BlendMode blendmode);
BlendFunc get_graya_blender(BlendMode blendmode);
BlendFunc get_indexed_blender(BlendMode blendmode);

template<typename ImageTraits>
BlendFunc get_blender(BlendMode mode)
{
  if constexpr (ImageTraits::color_mode == ColorMode::RGB)
    return get_rgba_blender(mode);
  if constexpr (ImageTraits::color_mode == ColorMode::GRAYSCALE)
    return get_graya_blender(mode);
  if constexpr (ImageTraits::color_mode == ColorMode::INDEXED)
    return get_indexed_blender(mode);
  return nullptr;
}

} // namespace doc

#endif
