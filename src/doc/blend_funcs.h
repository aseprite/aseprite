// Aseprite Document Library
// Copyright (c) 2001-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_BLEND_FUNCS_H_INCLUDED
#define DOC_BLEND_FUNCS_H_INCLUDED
#pragma once

#include "doc/blend_mode.h"
#include "doc/color.h"

namespace doc {

  typedef color_t (*BlendFunc)(color_t backdrop, color_t src, int opacity);

  color_t rgba_blender_normal(color_t backdrop, color_t src, int opacity = 255);
  color_t rgba_blender_merge(color_t backdrop, color_t src, int opacity);
  color_t rgba_blender_neg_bw(color_t backdrop, color_t src, int opacity);

  color_t graya_blender_normal(color_t backdrop, color_t src, int opacity = 255);
  color_t graya_blender_merge(color_t backdrop, color_t src, int opacity);
  color_t graya_blender_neg_bw(color_t backdrop, color_t src, int opacity);

  BlendFunc get_rgba_blender(BlendMode blendmode);
  BlendFunc get_graya_blender(BlendMode blendmode);
  BlendFunc get_indexed_blender(BlendMode blendmode);

} // namespace doc

#endif
