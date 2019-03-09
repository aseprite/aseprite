// Aseprite Document Library
// Copyright (C) 2019  Igara Studio S.A.
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

  color_t rgba_blender_src(color_t backdrop, color_t src, int opacity);
  color_t rgba_blender_merge(color_t backdrop, color_t src, int opacity);
  color_t rgba_blender_neg_bw(color_t backdrop, color_t src, int opacity);
  color_t rgba_blender_red_tint(color_t backdrop, color_t src, int opacity);
  color_t rgba_blender_blue_tint(color_t backdrop, color_t src, int opacity);
  color_t rgba_blender_normal(color_t backdrop, color_t src, int opacity = 255);
  color_t rgba_blender_normal_dst_over(color_t backdrop, color_t src, int opacity);
  color_t rgba_blender_multiply(color_t backdrop, color_t src, int opacity);
  color_t rgba_blender_screen(color_t backdrop, color_t src, int opacity);
  color_t rgba_blender_overlay(color_t backdrop, color_t src, int opacity);
  color_t rgba_blender_darken(color_t backdrop, color_t src, int opacity);
  color_t rgba_blender_lighten(color_t backdrop, color_t src, int opacity);
  color_t rgba_blender_color_dodge(color_t backdrop, color_t src, int opacity);
  color_t rgba_blender_color_burn(color_t backdrop, color_t src, int opacity);
  color_t rgba_blender_hard_light(color_t backdrop, color_t src, int opacity);
  color_t rgba_blender_soft_light(color_t backdrop, color_t src, int opacity);
  color_t rgba_blender_difference(color_t backdrop, color_t src, int opacity);
  color_t rgba_blender_exclusion(color_t backdrop, color_t src, int opacity);
  color_t rgba_blender_hsl_hue(color_t backdrop, color_t src, int opacity);
  color_t rgba_blender_hsl_saturation(color_t backdrop, color_t src, int opacity);
  color_t rgba_blender_hsl_color(color_t backdrop, color_t src, int opacity);
  color_t rgba_blender_hsl_luminosity(color_t backdrop, color_t src, int opacity);
  color_t rgba_blender_addition(color_t backdrop, color_t src, int opacity);
  color_t rgba_blender_subtract(color_t backdrop, color_t src, int opacity);
  color_t rgba_blender_divide(color_t backdrop, color_t src, int opacity);

  color_t graya_blender_src(color_t backdrop, color_t src, int opacity);
  color_t graya_blender_merge(color_t backdrop, color_t src, int opacity);
  color_t graya_blender_neg_bw(color_t backdrop, color_t src, int opacity);
  color_t graya_blender_normal(color_t backdrop, color_t src, int opacity = 255);
  color_t graya_blender_normal_dst_over(color_t backdrop, color_t src, int opacity);
  color_t graya_blender_multiply(color_t backdrop, color_t src, int opacity);
  color_t graya_blender_screen(color_t backdrop, color_t src, int opacity);
  color_t graya_blender_overlay(color_t backdrop, color_t src, int opacity);
  color_t graya_blender_darken(color_t backdrop, color_t src, int opacity);
  color_t graya_blender_lighten(color_t backdrop, color_t src, int opacity);
  color_t graya_blender_color_dodge(color_t backdrop, color_t src, int opacity);
  color_t graya_blender_color_burn(color_t backdrop, color_t src, int opacity);
  color_t graya_blender_hard_light(color_t backdrop, color_t src, int opacity);
  color_t graya_blender_soft_light(color_t backdrop, color_t src, int opacity);
  color_t graya_blender_difference(color_t backdrop, color_t src, int opacity);
  color_t graya_blender_exclusion(color_t backdrop, color_t src, int opacity);
  color_t graya_blender_addition(color_t backdrop, color_t src, int opacity);
  color_t graya_blender_subtract(color_t backdrop, color_t src, int opacity);
  color_t graya_blender_divide(color_t backdrop, color_t src, int opacity);

  color_t indexed_blender_src(color_t dst, color_t src, int opacity);

  BlendFunc get_rgba_blender(BlendMode blendmode, const bool newBlend);
  BlendFunc get_graya_blender(BlendMode blendmode, const bool newBlend);
  BlendFunc get_indexed_blender(BlendMode blendmode, const bool newBlend);

} // namespace doc

#endif
