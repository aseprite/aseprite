// Aseprite Document Library
// Copyright (c) 2001-2016 David Capello
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

#include <cmath>

namespace  {

#define blend_multiply(b, s, t)   (MUL_UN8((b), (s), (t)))
#define blend_screen(b, s, t)     ((b) + (s) - MUL_UN8((b), (s), (t)))
#define blend_overlay(b, s, t)    (blend_hard_light(s, b, t))
#define blend_darken(b, s)        (MIN((b), (s)))
#define blend_lighten(b, s)       (MAX((b), (s)))
#define blend_hard_light(b, s, t) ((s) < 128 ?                          \
                                   blend_multiply((b), (s)<<1, (t)):    \
                                   blend_screen((b), ((s)<<1)-255, (t)))
#define blend_difference(b, s)    (ABS((b) - (s)))
#define blend_exclusion(b, s, t)  ((t) = MUL_UN8((b), (s), (t)), ((b) + (s) - 2*(t)))

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
    d = ((16*b-12)*b+4)*b;
  else
    d = std::sqrt(b);

  if (s <= 0.5)
    r = b - (1.0 - 2.0*s) * b * (1.0 - b);
  else
    r = b - (1.0 - 2.0*s) * b * (1.0 - b);

  return (uint32_t)(r * 255 + 0.5);
}

} // annonymous namespace

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
    return src;
  else if (rgba_luma(backdrop) < 128)
    return rgba(255, 255, 255, 255);
  else
    return rgba(0, 0, 0, 255);
}

color_t rgba_blender_red_tint(color_t backdrop, color_t src, int opacity)
{
  int v = rgba_luma(src);
  src = rgba(v, 0, 0, rgba_geta(src));
  return rgba_blender_normal(backdrop, src, opacity);
}

color_t rgba_blender_blue_tint(color_t backdrop, color_t src, int opacity)
{
  int v = rgba_luma(src);
  src = rgba(0, 0, v, rgba_geta(src));
  return rgba_blender_normal(backdrop, src, opacity);
}

color_t rgba_blender_normal(color_t backdrop, color_t src, int opacity)
{
  int t;

  if ((backdrop & rgba_a_mask) == 0) {
    int a = rgba_geta(src);
    a = MUL_UN8(a, opacity, t);
    a <<= rgba_a_shift;
    return (src & rgba_rgb_mask) | a;
  }
  else if ((src & rgba_a_mask) == 0) {
    return backdrop;
  }

  int Br, Bg, Bb, Ba;
  int Sr, Sg, Sb, Sa;
  int Rr, Rg, Rb, Ra;

  Br = rgba_getr(backdrop);
  Bg = rgba_getg(backdrop);
  Bb = rgba_getb(backdrop);
  Ba = rgba_geta(backdrop);

  Sr = rgba_getr(src);
  Sg = rgba_getg(src);
  Sb = rgba_getb(src);
  Sa = rgba_geta(src);
  Sa = MUL_UN8(Sa, opacity, t);

  Ra = Ba + Sa - MUL_UN8(Ba, Sa, t);
  Rr = Br + (Sr-Br) * Sa / Ra;
  Rg = Bg + (Sg-Bg) * Sa / Ra;
  Rb = Bb + (Sb-Bb) * Sa / Ra;

  return rgba(Rr, Rg, Rb, Ra);
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
  return 0.3*r + 0.59*g + 0.11*b;
}

static double sat(double r, double g, double b)
{
  return MAX(r, MAX(g, b)) - MIN(r, MIN(g, b));
}

static void clip_color(double& r, double& g, double& b)
{
  double l = lum(r, g, b);
  double n = MIN(r, MIN(g, b));
  double x = MAX(r, MAX(g, b));

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
  double& min = MIN(r, MIN(g, b));
  double& mid = MID(r, g, b);
  double& max = MAX(r, MAX(g, b));

  if (max > min) {
    mid = ((mid - min)*s) / (max - min);
    max = s;
  }
  else
    mid = max = 0;

  min = 0;
}

color_t rgba_blender_hsl_hue(color_t backdrop, color_t src, int opacity)
{
  double r = rgba_getr(backdrop)/255.0;
  double g = rgba_getg(backdrop)/255.0;
  double b = rgba_getb(backdrop)/255.0;
  double s = sat(r, g, b);
  double l = lum(r, g, b);

  r = rgba_getr(src)/255.0;
  g = rgba_getg(src)/255.0;
  b = rgba_getb(src)/255.0;

  set_sat(r, g, b, s);
  set_lum(r, g, b, l);

  src = rgba(int(255.0*r), int(255.0*g), int(255.0*b), 0) | (src & rgba_a_mask);
  return rgba_blender_normal(backdrop, src, opacity);
}

color_t rgba_blender_hsl_saturation(color_t backdrop, color_t src, int opacity)
{
  double r = rgba_getr(src)/255.0;
  double g = rgba_getg(src)/255.0;
  double b = rgba_getb(src)/255.0;
  double s = sat(r, g, b);

  r = rgba_getr(backdrop)/255.0;
  g = rgba_getg(backdrop)/255.0;
  b = rgba_getb(backdrop)/255.0;
  double l = lum(r, g, b);

  set_sat(r, g, b, s);
  set_lum(r, g, b, l);

  src = rgba(int(255.0*r), int(255.0*g), int(255.0*b), 0) | (src & rgba_a_mask);
  return rgba_blender_normal(backdrop, src, opacity);
}

color_t rgba_blender_hsl_color(color_t backdrop, color_t src, int opacity)
{
  double r = rgba_getr(backdrop)/255.0;
  double g = rgba_getg(backdrop)/255.0;
  double b = rgba_getb(backdrop)/255.0;
  double l = lum(r, g, b);

  r = rgba_getr(src)/255.0;
  g = rgba_getg(src)/255.0;
  b = rgba_getb(src)/255.0;

  set_lum(r, g, b, l);

  src = rgba(int(255.0*r), int(255.0*g), int(255.0*b), 0) | (src & rgba_a_mask);
  return rgba_blender_normal(backdrop, src, opacity);
}

color_t rgba_blender_hsl_luminosity(color_t backdrop, color_t src, int opacity)
{
  double r = rgba_getr(src)/255.0;
  double g = rgba_getg(src)/255.0;
  double b = rgba_getb(src)/255.0;
  double l = lum(r, g, b);

  r = rgba_getr(backdrop)/255.0;
  g = rgba_getg(backdrop)/255.0;
  b = rgba_getb(backdrop)/255.0;

  set_lum(r, g, b, l);

  src = rgba(int(255.0*r), int(255.0*g), int(255.0*b), 0) | (src & rgba_a_mask);
  return rgba_blender_normal(backdrop, src, opacity);
}

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
    Rk = Bk + MUL_UN8((Sk-Bk), opacity, t);
  }
  Ra = Ba + MUL_UN8((Sa-Ba), opacity, t);
  if (Ra == 0)
    Rk = 0;

  return graya(Rk, Ra);
}

color_t graya_blender_neg_bw(color_t backdrop, color_t src, int opacity)
{
  if ((backdrop & graya_a_mask) == 0)
    return src;
  else if (graya_getv(backdrop) < 128)
    return graya(255, 255);
  else
    return graya(0, 255);
}

color_t graya_blender_normal(color_t backdrop, color_t src, int opacity)
{
  int t;

  if ((backdrop & graya_a_mask) == 0) {
    int a = graya_geta(src);
    a = MUL_UN8(a, opacity, t);
    a <<= graya_a_shift;
    return (src & 0xff) | a;
  }
  else if ((src & graya_a_mask) == 0)
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
  Rg = Bg + (Sg-Bg) * Sa / Ra;

  return graya(Rg, Ra);
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

    case BlendMode::NORMAL:         return rgba_blender_normal;
    case BlendMode::MULTIPLY:       return rgba_blender_multiply;
    case BlendMode::SCREEN:         return rgba_blender_screen;
    case BlendMode::OVERLAY:        return rgba_blender_overlay;
    case BlendMode::DARKEN:         return rgba_blender_darken;
    case BlendMode::LIGHTEN:        return rgba_blender_lighten;
    case BlendMode::COLOR_DODGE:    return rgba_blender_color_dodge;
    case BlendMode::COLOR_BURN:     return rgba_blender_color_burn;
    case BlendMode::HARD_LIGHT:     return rgba_blender_hard_light;
    case BlendMode::SOFT_LIGHT:     return rgba_blender_soft_light;
    case BlendMode::DIFFERENCE:     return rgba_blender_difference;
    case BlendMode::EXCLUSION:      return rgba_blender_exclusion;
    case BlendMode::HSL_HUE:        return rgba_blender_hsl_hue;
    case BlendMode::HSL_SATURATION: return rgba_blender_hsl_saturation;
    case BlendMode::HSL_COLOR:      return rgba_blender_hsl_color;
    case BlendMode::HSL_LUMINOSITY: return rgba_blender_hsl_luminosity;
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

    case BlendMode::NORMAL:         return graya_blender_normal;
    case BlendMode::MULTIPLY:       return graya_blender_multiply;
    case BlendMode::SCREEN:         return graya_blender_screen;
    case BlendMode::OVERLAY:        return graya_blender_overlay;
    case BlendMode::DARKEN:         return graya_blender_darken;
    case BlendMode::LIGHTEN:        return graya_blender_lighten;
    case BlendMode::COLOR_DODGE:    return graya_blender_color_dodge;
    case BlendMode::COLOR_BURN:     return graya_blender_color_burn;
    case BlendMode::HARD_LIGHT:     return graya_blender_hard_light;
    case BlendMode::SOFT_LIGHT:     return graya_blender_soft_light;
    case BlendMode::DIFFERENCE:     return graya_blender_difference;
    case BlendMode::EXCLUSION:      return graya_blender_exclusion;
    case BlendMode::HSL_HUE:        return graya_blender_normal;
    case BlendMode::HSL_SATURATION: return graya_blender_normal;
    case BlendMode::HSL_COLOR:      return graya_blender_normal;
    case BlendMode::HSL_LUMINOSITY: return graya_blender_normal;
  }
  ASSERT(false);
  return graya_blender_src;
}

BlendFunc get_indexed_blender(BlendMode blendmode)
{
  return indexed_blender_src;
}

} // namespace doc
