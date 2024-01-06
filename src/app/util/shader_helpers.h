// Aseprite
// Copyright (C) 2022  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UTIL_SHADER_HELPERS_H_INCLUDED
#define APP_UTIL_SHADER_HELPERS_H_INCLUDED
#pragma once

#if SK_ENABLE_SKSL

#include "app/color.h"
#include "gfx/color.h"

#include "include/core/SkM44.h"

namespace app {

// rgb_to_hsl() and hsv_to_hsl() functions by Sam Hocevar licensed
// under WTFPL (https://en.wikipedia.org/wiki/WTFPL)
// Source:
//   http://lolengine.net/blog/2013/07/27/rgb-to-hsv-in-glsl
//   https://stackoverflow.com/a/17897228/408239

inline constexpr char kRGB_to_HSV_sksl[] = R"(
half3 rgb_to_hsv(half3 c) {
 half4 K = half4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
 half4 p = mix(half4(c.bg, K.wz), half4(c.gb, K.xy), step(c.b, c.g));
 half4 q = mix(half4(p.xyw, c.r), half4(c.r, p.yzx), step(p.x, c.r));

 float d = q.x - min(q.w, q.y);
 float e = 1.0e-10;
 return half3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}
)";

inline constexpr char kHSV_to_RGB_sksl[] = R"(
half3 hsv_to_rgb(half3 c) {
 half4 K = half4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
 half3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
 return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}
)";

inline SkV4 gfxColor_to_SkV4(gfx::Color color) {
  return SkV4{float(gfx::getr(color) / 255.0),
              float(gfx::getg(color) / 255.0),
              float(gfx::getb(color) / 255.0),
              float(gfx::geta(color) / 255.0)};
}

inline SkV4 appColor_to_SkV4(const app::Color& color) {
  return SkV4{float(color.getRed() / 255.0),
              float(color.getGreen() / 255.0),
              float(color.getBlue() / 255.0),
              float(color.getAlpha() / 255.0)};
}

inline SkV4 appColorHsv_to_SkV4(const app::Color& color) {
  return SkV4{float(color.getHsvHue() / 360.0),
              float(color.getHsvSaturation()),
              float(color.getHsvValue()),
              float(color.getAlpha() / 255.0)};
}

inline SkV4 appColorHsl_to_SkV4(const app::Color& color) {
  return SkV4{float(color.getHslHue() / 360.0),
              float(color.getHslSaturation()),
              float(color.getHslLightness()),
              float(color.getAlpha() / 255.0)};
}

} // namespace app

#endif

#endif
