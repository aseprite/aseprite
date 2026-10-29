// Aseprite Document Library
// Copyright (c) 2019-present Igara Studio S.A.
// Copyright (c) 2001-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.
//
// --
//
// Some references about alpha compositing and blend modes:
//
//   http://dev.w3.org/fxtf/compositing-1/
//   http://www.adobe.com/devnet/pdf/pdf_reference.html
//

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/blend_funcs.h"

#include "base/debug.h"
#include "doc/blend_internals.h"

#include <algorithm>
#include <cmath>

namespace {

#define blend_multiply(b, s, t) (MUL_UN8((b), (s), (t)))
#define blend_screen(b, s, t)   ((b) + (s) - MUL_UN8((b), (s), (t)))
#define blend_overlay(b, s, t)  (blend_hard_light(s, b, t))
#define blend_darken(b, s)      (std::min((b), (s)))
#define blend_lighten(b, s)     (std::max((b), (s)))
#define blend_hard_light(b, s, t)                                                                  \
  ((s) < 128 ? blend_multiply((b), (s) << 1, (t)) : blend_screen((b), ((s) << 1) - 255, (t)))
#define blend_difference(b, s)   (ABS((b) - (s)))
#define blend_exclusion(b, s, t) ((t) = MUL_UN8((b), (s), (t)), ((b) + (s) - 2 * (t)))

#define RGBA_BLENDER_OR_NORMAL(name)                                                               \
  color_t rgba_blender_##name##_or_normal(color_t backdrop, color_t src, int opacity)              \
  {                                                                                                \
    if (backdrop & rgba_a_mask) {                                                                  \
      color_t normal = rgba_blender_normal(backdrop, src, opacity);                                \
      color_t blend = rgba_blender_##name(backdrop, src, opacity);                                 \
      int Ba = rgba_geta(backdrop);                                                                \
      return rgba_blender_merge(normal, blend, Ba);                                                \
    }                                                                                              \
    else                                                                                           \
      return rgba_blender_normal(backdrop, src, opacity);                                          \
  }

#define GRAYA_BLENDER_OR_NORMAL(name)                                                              \
  color_t graya_blender_##name##_or_normal(color_t backdrop, color_t src, int opacity)             \
  {                                                                                                \
    if (backdrop & graya_a_mask) {                                                                 \
      color_t normal = graya_blender_normal(backdrop, src, opacity);                               \
      color_t blend = graya_blender_##name(backdrop, src, opacity);                                \
      int Ba = graya_geta(backdrop);                                                               \
      return graya_blender_merge(normal, blend, Ba);                                               \
    }                                                                                              \
    else                                                                                           \
      return graya_blender_normal(backdrop, src, opacity);                                         \
  }

inline uint32_t blend_divide(uint32_t b, uint32_t s)
{
  if (b == 0)
    return 0;
  else if (b >= s)
    return 255;
  else
    return DIV_UN8(b, s); // return b / s
}

inline uint32_t blend_color_dodge(uint32_t b, uint32_t s)
{
  if (b == 0)
    return 0;

  s = (255 - s);
  if (b >= s)
    return 255;
  else
    return DIV_UN8(b, s); // return b / (1-s)
}

inline uint32_t blend_color_burn(uint32_t b, uint32_t s)
{
  if (b == 255)
    return 255;

  b = (255 - b);
  if (b >= s)
    return 0;
  else
    return 255 - DIV_UN8(b, s); // return 1 - ((1-b)/s)
}

inline uint32_t blend_soft_light(uint32_t _b, uint32_t _s)
{
  double b = _b / 255.0;
  double s = _s / 255.0;
  double r, d;

  if (b <= 0.25)
    d = ((16 * b - 12) * b + 4) * b;
  else
    d = std::sqrt(b);

  if (s <= 0.5)
    r = b - (1.0 - 2.0 * s) * b * (1.0 - b);
  else
    r = b + (2.0 * s - 1.0) * (d - b);

  return (uint32_t)(r * 255 + 0.5);
}

} // namespace

namespace doc {

//////////////////////////////////////////////////////////////////////
// RGB blenders

color_t rgba_blender_src(color_t backdrop, color_t src, int opacity)
{
  return src;
}

color_t rgba_blender_merge(color_t backdrop, color_t src, int opacity)
{
  int Br, Bg, Bb, Ba;
  int Sr, Sg, Sb, Sa;
  int Rr, Rg, Rb, Ra;
  int t;

  Br = rgba_getr(backdrop);
  Bg = rgba_getg(backdrop);
  Bb = rgba_getb(backdrop);
  Ba = rgba_geta(backdrop);

  Sr = rgba_getr(src);
  Sg = rgba_getg(src);
  Sb = rgba_getb(src);
  Sa = rgba_geta(src);

  if (Ba == 0) {
    Rr = Sr;
    Rg = Sg;
    Rb = Sb;
  }
  else if (Sa == 0) {
    Rr = Br;
    Rg = Bg;
    Rb = Bb;
  }
  else {
    Rr = Br + MUL_UN8((Sr - Br), opacity, t);
    Rg = Bg + MUL_UN8((Sg - Bg), opacity, t);
    Rb = Bb + MUL_UN8((Sb - Bb), opacity, t);
  }
  Ra = Ba + MUL_UN8((Sa - Ba), opacity, t);
  if (Ra == 0)
    Rr = Rg = Rb = 0;

  return rgba(Rr, Rg, Rb, Ra);
}

color_t rgba_blender_neg_bw(color_t backdrop, color_t src, int opacity)
{
  if (!(backdrop & rgba_a_mask))
    return rgba(0, 0, 0, 255);
  else if (rgba_luma(backdrop) < 128)
    return rgba(255, 255, 255, 255);
  else
    return rgba(0, 0, 0, 255);
}

color_t rgba_blender_red_tint(color_t backdrop, color_t src, int opacity)
{
  int v = rgba_luma(src);
  src = rgba((255 + v) / 2, v / 2, v / 2, rgba_geta(src));
  return rgba_blender_normal(backdrop, src, opacity);
}

color_t rgba_blender_blue_tint(color_t backdrop, color_t src, int opacity)
{
  int v = rgba_luma(src);
  src = rgba(v / 2, v / 2, (255 + v) / 2, rgba_geta(src));
  return rgba_blender_normal(backdrop, src, opacity);
}

color_t rgba_blender_normal(color_t backdrop, color_t src, int opacity)
{
  int t;

  if (!(backdrop & rgba_a_mask)) {
    int a = rgba_geta(src);
    a = MUL_UN8(a, opacity, t);
    a <<= rgba_a_shift;
    return (src & rgba_rgb_mask) | a;
  }
  else if (!(src & rgba_a_mask)) {
    return backdrop;
  }

  const int Br = rgba_getr(backdrop);
  const int Bg = rgba_getg(backdrop);
  const int Bb = rgba_getb(backdrop);
  const int Ba = rgba_geta(backdrop);

  const int Sr = rgba_getr(src);
  const int Sg = rgba_getg(src);
  const int Sb = rgba_getb(src);
  int Sa = rgba_geta(src);
  Sa = MUL_UN8(Sa, opacity, t);

  // Ra = Sa + Ba*(1-Sa)
  //    = Sa + Ba - Ba*Sa
  const int Ra = Sa + Ba - MUL_UN8(Ba, Sa, t);

  // Ra = Sa + Ba*(1-Sa)
  // Ba = (Ra-Sa) / (1-Sa)
  // Rc = (Sc*Sa + Bc*Ba*(1-Sa)) / Ra                Replacing Ba with (Ra-Sa) / (1-Sa)...
  //    = (Sc*Sa + Bc*(Ra-Sa)/(1-Sa)*(1-Sa)) / Ra
  //    = (Sc*Sa + Bc*(Ra-Sa)) / Ra
  //    = Sc*Sa/Ra + Bc*Ra/Ra - Bc*Sa/Ra
  //    = Sc*Sa/Ra + Bc - Bc*Sa/Ra
  //    = Bc + (Sc-Bc)*Sa/Ra
  const int Rr = Br + (Sr - Br) * Sa / Ra;
  const int Rg = Bg + (Sg - Bg) * Sa / Ra;
  const int Rb = Bb + (Sb - Bb) * Sa / Ra;

  return rgba(Rr, Rg, Rb, Ra);
}

color_t rgba_blender_normal_dst_over(color_t backdrop, color_t src, int opacity)
{
  int t;
  int Sa = MUL_UN8(rgba_geta(src), opacity, t);
  src = (src & rgba_rgb_mask) | (Sa << rgba_a_shift);
  return rgba_blender_normal(src, backdrop);
}

color_t rgba_blender_multiply(color_t backdrop, color_t src, int opacity)
{
  int t;
  int r = blend_multiply(rgba_getr(backdrop), rgba_getr(src), t);
  int g = blend_multiply(rgba_getg(backdrop), rgba_getg(src), t);
  int b = blend_multiply(rgba_getb(backdrop), rgba_getb(src), t);
  src = rgba(r, g, b, 0) | (src & rgba_a_mask);
  return rgba_blender_normal(backdrop, src, opacity);
}

color_t rgba_blender_screen(color_t backdrop, color_t src, int opacity)
{
  int t;
  int r = blend_screen(rgba_getr(backdrop), rgba_getr(src), t);
  int g = blend_screen(rgba_getg(backdrop), rgba_getg(src), t);
  int b = blend_screen(rgba_getb(backdrop), rgba_getb(src), t);
  src = rgba(r, g, b, 0) | (src & rgba_a_mask);
  return rgba_blender_normal(backdrop, src, opacity);
}

color_t rgba_blender_overlay(color_t backdrop, color_t src, int opacity)
{
  int t;
  int r = blend_overlay(rgba_getr(backdrop), rgba_getr(src), t);
  int g = blend_overlay(rgba_getg(backdrop), rgba_getg(src), t);
  int b = blend_overlay(rgba_getb(backdrop), rgba_getb(src), t);
  src = rgba(r, g, b, 0) | (src & rgba_a_mask);
  return rgba_blender_normal(backdrop, src, opacity);
}

color_t rgba_blender_darken(color_t backdrop, color_t src, int opacity)
{
  int r = blend_darken(rgba_getr(backdrop), rgba_getr(src));
  int g = blend_darken(rgba_getg(backdrop), rgba_getg(src));
  int b = blend_darken(rgba_getb(backdrop), rgba_getb(src));
  src = rgba(r, g, b, 0) | (src & rgba_a_mask);
  return rgba_blender_normal(backdrop, src, opacity);
}

color_t rgba_blender_lighten(color_t backdrop, color_t src, int opacity)
{
  int r = blend_lighten(rgba_getr(backdrop), rgba_getr(src));
  int g = blend_lighten(rgba_getg(backdrop), rgba_getg(src));
  int b = blend_lighten(rgba_getb(backdrop), rgba_getb(src));
  src = rgba(r, g, b, 0) | (src & rgba_a_mask);
  return rgba_blender_normal(backdrop, src, opacity);
}

color_t rgba_blender_color_dodge(color_t backdrop, color_t src, int opacity)
{
  int r = blend_color_dodge(rgba_getr(backdrop), rgba_getr(src));
  int g = blend_color_dodge(rgba_getg(backdrop), rgba_getg(src));
  int b = blend_color_dodge(rgba_getb(backdrop), rgba_getb(src));
  src = rgba(r, g, b, 0) | (src & rgba_a_mask);
  return rgba_blender_normal(backdrop, src, opacity);
}

color_t rgba_blender_color_burn(color_t backdrop, color_t src, int opacity)
{
  int r = blend_color_burn(rgba_getr(backdrop), rgba_getr(src));
  int g = blend_color_burn(rgba_getg(backdrop), rgba_getg(src));
  int b = blend_color_burn(rgba_getb(backdrop), rgba_getb(src));
  src = rgba(r, g, b, 0) | (src & rgba_a_mask);
  return rgba_blender_normal(backdrop, src, opacity);
}

color_t rgba_blender_hard_light(color_t backdrop, color_t src, int opacity)
{
  int t;
  int r = blend_hard_light(rgba_getr(backdrop), rgba_getr(src), t);
  int g = blend_hard_light(rgba_getg(backdrop), rgba_getg(src), t);
  int b = blend_hard_light(rgba_getb(backdrop), rgba_getb(src), t);
  src = rgba(r, g, b, 0) | (src & rgba_a_mask);
  return rgba_blender_normal(backdrop, src, opacity);
}

color_t rgba_blender_soft_light(color_t backdrop, color_t src, int opacity)
{
  int r = blend_soft_light(rgba_getr(backdrop), rgba_getr(src));
  int g = blend_soft_light(rgba_getg(backdrop), rgba_getg(src));
  int b = blend_soft_light(rgba_getb(backdrop), rgba_getb(src));
  src = rgba(r, g, b, 0) | (src & rgba_a_mask);
  return rgba_blender_normal(backdrop, src, opacity);
}

color_t rgba_blender_difference(color_t backdrop, color_t src, int opacity)
{
  int r = blend_difference(rgba_getr(backdrop), rgba_getr(src));
  int g = blend_difference(rgba_getg(backdrop), rgba_getg(src));
  int b = blend_difference(rgba_getb(backdrop), rgba_getb(src));
  src = rgba(r, g, b, 0) | (src & rgba_a_mask);
  return rgba_blender_normal(backdrop, src, opacity);
}

color_t rgba_blender_exclusion(color_t backdrop, color_t src, int opacity)
{
  int t;
  int r = blend_exclusion(rgba_getr(backdrop), rgba_getr(src), t);
  int g = blend_exclusion(rgba_getg(backdrop), rgba_getg(src), t);
  int b = blend_exclusion(rgba_getb(backdrop), rgba_getb(src), t);
  src = rgba(r, g, b, 0) | (src & rgba_a_mask);
  return rgba_blender_normal(backdrop, src, opacity);
}

//////////////////////////////////////////////////////////////////////
// HSV blenders

static double lum(double r, double g, double b)
{
  return 0.3 * r + 0.59 * g + 0.11 * b;
}

static double sat(double r, double g, double b)
{
  return std::max(r, std::max(g, b)) - std::min(r, std::min(g, b));
}

static void clip_color(double& r, double& g, double& b)
{
  double l = lum(r, g, b);
  double n = std::min(r, std::min(g, b));
  double x = std::max(r, std::max(g, b));

  if (n < 0) {
    r = l + (((r - l) * l) / (l - n));
    g = l + (((g - l) * l) / (l - n));
    b = l + (((b - l) * l) / (l - n));
  }

  if (x > 1) {
    r = l + (((r - l) * (1 - l)) / (x - l));
    g = l + (((g - l) * (1 - l)) / (x - l));
    b = l + (((b - l) * (1 - l)) / (x - l));
  }
}

static void set_lum(double& r, double& g, double& b, double l)
{
  double d = l - lum(r, g, b);
  r += d;
  g += d;
  b += d;
  clip_color(r, g, b);
}

static void set_sat(double& r, double& g, double& b, double s)
{
  const double minv = std::min(std::min(r, g), b);
  const double maxv = std::max(std::max(r, g), b);
  const double range = maxv - minv;

  if (range > 0.0) {
    r = ((r - minv) * s) / range;
    g = ((g - minv) * s) / range;
    b = ((b - minv) * s) / range;
  }
  else
    r = g = b = 0.0;
}

color_t rgba_blender_hsl_hue(color_t backdrop, color_t src, int opacity)
{
  double r = rgba_getr(backdrop) / 255.0;
  double g = rgba_getg(backdrop) / 255.0;
  double b = rgba_getb(backdrop) / 255.0;
  double s = sat(r, g, b);
  double l = lum(r, g, b);

  r = rgba_getr(src) / 255.0;
  g = rgba_getg(src) / 255.0;
  b = rgba_getb(src) / 255.0;

  set_sat(r, g, b, s);
  set_lum(r, g, b, l);

  src = rgba(int(255.0 * r), int(255.0 * g), int(255.0 * b), 0) | (src & rgba_a_mask);
  return rgba_blender_normal(backdrop, src, opacity);
}

color_t rgba_blender_hsl_saturation(color_t backdrop, color_t src, int opacity)
{
  double r = rgba_getr(src) / 255.0;
  double g = rgba_getg(src) / 255.0;
  double b = rgba_getb(src) / 255.0;
  double s = sat(r, g, b);

  r = rgba_getr(backdrop) / 255.0;
  g = rgba_getg(backdrop) / 255.0;
  b = rgba_getb(backdrop) / 255.0;
  double l = lum(r, g, b);

  set_sat(r, g, b, s);
  set_lum(r, g, b, l);

  src = rgba(int(255.0 * r), int(255.0 * g), int(255.0 * b), 0) | (src & rgba_a_mask);
  return rgba_blender_normal(backdrop, src, opacity);
}

color_t rgba_blender_hsl_color(color_t backdrop, color_t src, int opacity)
{
  double r = rgba_getr(backdrop) / 255.0;
  double g = rgba_getg(backdrop) / 255.0;
  double b = rgba_getb(backdrop) / 255.0;
  double l = lum(r, g, b);

  r = rgba_getr(src) / 255.0;
  g = rgba_getg(src) / 255.0;
  b = rgba_getb(src) / 255.0;

  set_lum(r, g, b, l);

  src = rgba(int(255.0 * r), int(255.0 * g), int(255.0 * b), 0) | (src & rgba_a_mask);
  return rgba_blender_normal(backdrop, src, opacity);
}

color_t rgba_blender_hsl_luminosity(color_t backdrop, color_t src, int opacity)
{
  double r = rgba_getr(src) / 255.0;
  double g = rgba_getg(src) / 255.0;
  double b = rgba_getb(src) / 255.0;
  double l = lum(r, g, b);

  r = rgba_getr(backdrop) / 255.0;
  g = rgba_getg(backdrop) / 255.0;
  b = rgba_getb(backdrop) / 255.0;

  set_lum(r, g, b, l);

  src = rgba(int(255.0 * r), int(255.0 * g), int(255.0 * b), 0) | (src & rgba_a_mask);
  return rgba_blender_normal(backdrop, src, opacity);
}

color_t rgba_blender_addition(color_t backdrop, color_t src, int opacity)
{
  int r = rgba_getr(backdrop) + rgba_getr(src);
  int g = rgba_getg(backdrop) + rgba_getg(src);
  int b = rgba_getb(backdrop) + rgba_getb(src);
  src = rgba(std::min(r, 255), std::min(g, 255), std::min(b, 255), 0) | (src & rgba_a_mask);
  return rgba_blender_normal(backdrop, src, opacity);
}

color_t rgba_blender_subtract(color_t backdrop, color_t src, int opacity)
{
  int r = rgba_getr(backdrop) - rgba_getr(src);
  int g = rgba_getg(backdrop) - rgba_getg(src);
  int b = rgba_getb(backdrop) - rgba_getb(src);
  src = rgba(std::max(r, 0), std::max(g, 0), std::max(b, 0), 0) | (src & rgba_a_mask);
  return rgba_blender_normal(backdrop, src, opacity);
}

color_t rgba_blender_divide(color_t backdrop, color_t src, int opacity)
{
  int r = blend_divide(rgba_getr(backdrop), rgba_getr(src));
  int g = blend_divide(rgba_getg(backdrop), rgba_getg(src));
  int b = blend_divide(rgba_getb(backdrop), rgba_getb(src));
  src = rgba(r, g, b, 0) | (src & rgba_a_mask);
  return rgba_blender_normal(backdrop, src, opacity);
}

RGBA_BLENDER_OR_NORMAL(multiply)
RGBA_BLENDER_OR_NORMAL(screen)
RGBA_BLENDER_OR_NORMAL(overlay)
RGBA_BLENDER_OR_NORMAL(darken)
RGBA_BLENDER_OR_NORMAL(lighten)
RGBA_BLENDER_OR_NORMAL(color_dodge)
RGBA_BLENDER_OR_NORMAL(color_burn)
RGBA_BLENDER_OR_NORMAL(hard_light)
RGBA_BLENDER_OR_NORMAL(soft_light)
RGBA_BLENDER_OR_NORMAL(difference)
RGBA_BLENDER_OR_NORMAL(exclusion)
RGBA_BLENDER_OR_NORMAL(hsl_color)
RGBA_BLENDER_OR_NORMAL(hsl_hue)
RGBA_BLENDER_OR_NORMAL(hsl_saturation)
RGBA_BLENDER_OR_NORMAL(hsl_luminosity)
RGBA_BLENDER_OR_NORMAL(addition)
RGBA_BLENDER_OR_NORMAL(subtract)
RGBA_BLENDER_OR_NORMAL(divide)

//////////////////////////////////////////////////////////////////////
// GRAY blenders

color_t graya_blender_src(color_t backdrop, color_t src, int opacity)
{
  return src;
}

color_t graya_blender_merge(color_t backdrop, color_t src, int opacity)
{
  int Bk, Ba;
  int Sk, Sa;
  int Rk, Ra;
  int t;

  Bk = graya_getv(backdrop);
  Ba = graya_geta(backdrop);

  Sk = graya_getv(src);
  Sa = graya_geta(src);

  if (Ba == 0) {
    Rk = Sk;
  }
  else if (Sa == 0) {
    Rk = Bk;
  }
  else {
    Rk = Bk + MUL_UN8((Sk - Bk), opacity, t);
  }
  Ra = Ba + MUL_UN8((Sa - Ba), opacity, t);
  if (Ra == 0)
    Rk = 0;

  return graya(Rk, Ra);
}

color_t graya_blender_neg_bw(color_t backdrop, color_t src, int opacity)
{
  if (!(backdrop & graya_a_mask))
    return graya(0, 255);
  else if (graya_getv(backdrop) < 128)
    return graya(255, 255);
  else
    return graya(0, 255);
}

color_t graya_blender_normal(color_t backdrop, color_t src, int opacity)
{
  int t;

  if (!(backdrop & graya_a_mask)) {
    int a = graya_geta(src);
    a = MUL_UN8(a, opacity, t);
    a <<= graya_a_shift;
    return (src & 0xff) | a;
  }
  else if (!(src & graya_a_mask))
    return backdrop;

  int Bg, Ba;
  int Sg, Sa;
  int Rg, Ra;

  Bg = graya_getv(backdrop);
  Ba = graya_geta(backdrop);

  Sg = graya_getv(src);
  Sa = graya_geta(src);
  Sa = MUL_UN8(Sa, opacity, t);

  Ra = Ba + Sa - MUL_UN8(Ba, Sa, t);
  Rg = Bg + (Sg - Bg) * Sa / Ra;

  return graya(Rg, Ra);
}

color_t graya_blender_normal_dst_over(color_t backdrop, color_t src, int opacity)
{
  int t;
  int Sa = MUL_UN8(graya_geta(src), opacity, t);
  src = (src & graya_v_mask) | (Sa << graya_a_shift);
  return graya_blender_normal(src, backdrop);
}

color_t graya_blender_multiply(color_t backdrop, color_t src, int opacity)
{
  int t;
  int v = blend_multiply(graya_getv(backdrop), graya_getv(src), t);
  src = graya(v, 0) | (src & graya_a_mask);
  return graya_blender_normal(backdrop, src, opacity);
}

color_t graya_blender_screen(color_t backdrop, color_t src, int opacity)
{
  int t;
  int v = blend_screen(graya_getv(backdrop), graya_getv(src), t);
  src = graya(v, 0) | (src & graya_a_mask);
  return graya_blender_normal(backdrop, src, opacity);
}

color_t graya_blender_overlay(color_t backdrop, color_t src, int opacity)
{
  int t;
  int v = blend_overlay(graya_getv(backdrop), graya_getv(src), t);
  src = graya(v, 0) | (src & graya_a_mask);
  return graya_blender_normal(backdrop, src, opacity);
}

color_t graya_blender_darken(color_t backdrop, color_t src, int opacity)
{
  int v = blend_darken(graya_getv(backdrop), graya_getv(src));
  src = graya(v, 0) | (src & graya_a_mask);
  return graya_blender_normal(backdrop, src, opacity);
}

color_t graya_blender_lighten(color_t backdrop, color_t src, int opacity)
{
  int v = blend_lighten(graya_getv(backdrop), graya_getv(src));
  src = graya(v, 0) | (src & graya_a_mask);
  return graya_blender_normal(backdrop, src, opacity);
}

color_t graya_blender_color_dodge(color_t backdrop, color_t src, int opacity)
{
  int v = blend_color_dodge(graya_getv(backdrop), graya_getv(src));
  src = graya(v, 0) | (src & graya_a_mask);
  return graya_blender_normal(backdrop, src, opacity);
}

color_t graya_blender_color_burn(color_t backdrop, color_t src, int opacity)
{
  int v = blend_color_burn(graya_getv(backdrop), graya_getv(src));
  src = graya(v, 0) | (src & graya_a_mask);
  return graya_blender_normal(backdrop, src, opacity);
}

color_t graya_blender_hard_light(color_t backdrop, color_t src, int opacity)
{
  int t;
  int v = blend_hard_light(graya_getv(backdrop), graya_getv(src), t);
  src = graya(v, 0) | (src & graya_a_mask);
  return graya_blender_normal(backdrop, src, opacity);
}

color_t graya_blender_soft_light(color_t backdrop, color_t src, int opacity)
{
  int v = blend_soft_light(graya_getv(backdrop), graya_getv(src));
  src = graya(v, 0) | (src & graya_a_mask);
  return graya_blender_normal(backdrop, src, opacity);
}

color_t graya_blender_difference(color_t backdrop, color_t src, int opacity)
{
  int v = blend_difference(graya_getv(backdrop), graya_getv(src));
  src = graya(v, 0) | (src & graya_a_mask);
  return graya_blender_normal(backdrop, src, opacity);
}

color_t graya_blender_exclusion(color_t backdrop, color_t src, int opacity)
{
  int t;
  int v = blend_exclusion(graya_getv(backdrop), graya_getv(src), t);
  src = graya(v, 0) | (src & graya_a_mask);
  return graya_blender_normal(backdrop, src, opacity);
}

color_t graya_blender_addition(color_t backdrop, color_t src, int opacity)
{
  int v = graya_getv(backdrop) + graya_getv(src);
  src = graya(std::min(v, 255), 0) | (src & graya_a_mask);
  return graya_blender_normal(backdrop, src, opacity);
}

color_t graya_blender_subtract(color_t backdrop, color_t src, int opacity)
{
  int v = graya_getv(backdrop) - graya_getv(src);
  src = graya(std::max(v, 0), 0) | (src & graya_a_mask);
  return graya_blender_normal(backdrop, src, opacity);
}

color_t graya_blender_divide(color_t backdrop, color_t src, int opacity)
{
  int v = blend_divide(graya_getv(backdrop), graya_getv(src));
  src = graya(v, 0) | (src & graya_a_mask);
  return graya_blender_normal(backdrop, src, opacity);
}

GRAYA_BLENDER_OR_NORMAL(multiply)
GRAYA_BLENDER_OR_NORMAL(screen)
GRAYA_BLENDER_OR_NORMAL(overlay)
GRAYA_BLENDER_OR_NORMAL(darken)
GRAYA_BLENDER_OR_NORMAL(lighten)
GRAYA_BLENDER_OR_NORMAL(color_dodge)
GRAYA_BLENDER_OR_NORMAL(color_burn)
GRAYA_BLENDER_OR_NORMAL(hard_light)
GRAYA_BLENDER_OR_NORMAL(soft_light)
GRAYA_BLENDER_OR_NORMAL(difference)
GRAYA_BLENDER_OR_NORMAL(exclusion)
GRAYA_BLENDER_OR_NORMAL(addition)
GRAYA_BLENDER_OR_NORMAL(subtract)
GRAYA_BLENDER_OR_NORMAL(divide)

//////////////////////////////////////////////////////////////////////
// indexed

color_t indexed_blender_src(color_t dst, color_t src, int opacity)
{
  return src;
}

//////////////////////////////////////////////////////////////////////
// getters

BlendFunc get_rgba_blender(BlendMode blendmode)
{
  switch (blendmode) {
    case BlendMode::SRC:            return rgba_blender_src;
    case BlendMode::MERGE:          return rgba_blender_merge;
    case BlendMode::NEG_BW:         return rgba_blender_neg_bw;
    case BlendMode::RED_TINT:       return rgba_blender_red_tint;
    case BlendMode::BLUE_TINT:      return rgba_blender_blue_tint;
    case BlendMode::DST_OVER:       return rgba_blender_normal_dst_over;

    case BlendMode::NORMAL:         return rgba_blender_normal;
    case BlendMode::MULTIPLY:       return rgba_blender_multiply_or_normal;
    case BlendMode::SCREEN:         return rgba_blender_screen_or_normal;
    case BlendMode::OVERLAY:        return rgba_blender_overlay_or_normal;
    case BlendMode::DARKEN:         return rgba_blender_darken_or_normal;
    case BlendMode::LIGHTEN:        return rgba_blender_lighten_or_normal;
    case BlendMode::COLOR_DODGE:    return rgba_blender_color_dodge_or_normal;
    case BlendMode::COLOR_BURN:     return rgba_blender_color_burn_or_normal;
    case BlendMode::HARD_LIGHT:     return rgba_blender_hard_light_or_normal;
    case BlendMode::SOFT_LIGHT:     return rgba_blender_soft_light_or_normal;
    case BlendMode::DIFFERENCE:     return rgba_blender_difference_or_normal;
    case BlendMode::EXCLUSION:      return rgba_blender_exclusion_or_normal;
    case BlendMode::HSL_HUE:        return rgba_blender_hsl_hue_or_normal;
    case BlendMode::HSL_SATURATION: return rgba_blender_hsl_saturation_or_normal;
    case BlendMode::HSL_COLOR:      return rgba_blender_hsl_color_or_normal;
    case BlendMode::HSL_LUMINOSITY: return rgba_blender_hsl_luminosity_or_normal;
    case BlendMode::ADDITION:       return rgba_blender_addition_or_normal;
    case BlendMode::SUBTRACT:       return rgba_blender_subtract_or_normal;
    case BlendMode::DIVIDE:         return rgba_blender_divide_or_normal;
  }
  ASSERT(false);
  return rgba_blender_src;
}

BlendFunc get_graya_blender(BlendMode blendmode)
{
  switch (blendmode) {
    case BlendMode::SRC:            return graya_blender_src;
    case BlendMode::MERGE:          return graya_blender_merge;
    case BlendMode::NEG_BW:         return graya_blender_neg_bw;
    case BlendMode::RED_TINT:       return graya_blender_normal;
    case BlendMode::BLUE_TINT:      return graya_blender_normal;
    case BlendMode::DST_OVER:       return graya_blender_normal_dst_over;

    case BlendMode::NORMAL:         return graya_blender_normal;
    case BlendMode::MULTIPLY:       return graya_blender_multiply_or_normal;
    case BlendMode::SCREEN:         return graya_blender_screen_or_normal;
    case BlendMode::OVERLAY:        return graya_blender_overlay_or_normal;
    case BlendMode::DARKEN:         return graya_blender_darken_or_normal;
    case BlendMode::LIGHTEN:        return graya_blender_lighten_or_normal;
    case BlendMode::COLOR_DODGE:    return graya_blender_color_dodge_or_normal;
    case BlendMode::COLOR_BURN:     return graya_blender_color_burn_or_normal;
    case BlendMode::HARD_LIGHT:     return graya_blender_hard_light_or_normal;
    case BlendMode::SOFT_LIGHT:     return graya_blender_soft_light_or_normal;
    case BlendMode::DIFFERENCE:     return graya_blender_difference_or_normal;
    case BlendMode::EXCLUSION:      return graya_blender_exclusion_or_normal;
    case BlendMode::HSL_HUE:        return graya_blender_normal;
    case BlendMode::HSL_SATURATION: return graya_blender_normal;
    case BlendMode::HSL_COLOR:      return graya_blender_normal;
    case BlendMode::HSL_LUMINOSITY: return graya_blender_normal;
    case BlendMode::ADDITION:       return graya_blender_exclusion_or_normal;
    case BlendMode::SUBTRACT:       return graya_blender_subtract_or_normal;
    case BlendMode::DIVIDE:         return graya_blender_divide_or_normal;
  }
  ASSERT(false);
  return graya_blender_src;
}

BlendFunc get_indexed_blender(BlendMode blendmode)
{
  return indexed_blender_src;
}

} // namespace doc
