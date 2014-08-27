/* Aseprite
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "raster/blend.h"
#include "raster/image.h"

namespace raster {

BLEND_COLOR rgba_blenders[] =
{
  rgba_blend_normal,
  rgba_blend_copy,
  rgba_blend_merge,
  rgba_blend_red_tint,
  rgba_blend_blue_tint,
  rgba_blend_blackandwhite,
};

BLEND_COLOR graya_blenders[] =
{
  graya_blend_normal,
  graya_blend_copy,
  graya_blend_copy,
  graya_blend_copy,
  graya_blend_copy,
  graya_blend_blackandwhite,
};

/**********************************************************************/
/* RGB blenders                                                       */
/**********************************************************************/

int rgba_blend_normal(int back, int front, int opacity)
{
  int t;

  if ((back & 0xff000000) == 0) {
    return
      (front & 0xffffff) |
      (INT_MULT(rgba_geta(front), opacity, t) << rgba_a_shift);
  }
  else if ((front & 0xff000000) == 0) {
    return back;
  }
  else {
    int B_r, B_g, B_b, B_a;
    int F_r, F_g, F_b, F_a;
    int D_r, D_g, D_b, D_a;

    B_r = rgba_getr(back);
    B_g = rgba_getg(back);
    B_b = rgba_getb(back);
    B_a = rgba_geta(back);

    F_r = rgba_getr(front);
    F_g = rgba_getg(front);
    F_b = rgba_getb(front);
    F_a = rgba_geta(front);
    F_a = INT_MULT(F_a, opacity, t);

    D_a = B_a + F_a - INT_MULT(B_a, F_a, t);
    D_r = B_r + (F_r-B_r) * F_a / D_a;
    D_g = B_g + (F_g-B_g) * F_a / D_a;
    D_b = B_b + (F_b-B_b) * F_a / D_a;

    return rgba(D_r, D_g, D_b, D_a);
  }
}

int rgba_blend_copy(int back, int front, int opacity)
{
  return front;
}

int rgba_blend_forpath(int back, int front, int opacity)
{
  int t, F_r, F_g, F_b, F_a;
  
  F_r = rgba_getr(front);
  F_g = rgba_getg(front);
  F_b = rgba_getb(front);
  F_a = rgba_geta(front);
  F_a = INT_MULT(F_a, opacity, t);

  return rgba(F_r, F_g, F_b, F_a);
}

int rgba_blend_merge(int back, int front, int opacity)
{
  int B_r, B_g, B_b, B_a;
  int F_r, F_g, F_b, F_a;
  int D_r, D_g, D_b, D_a;

  B_r = rgba_getr(back);
  B_g = rgba_getg(back);
  B_b = rgba_getb(back);
  B_a = rgba_geta(back);

  F_r = rgba_getr(front);
  F_g = rgba_getg(front);
  F_b = rgba_getb(front);
  F_a = rgba_geta(front);

  if (B_a == 0) {
    D_r = F_r;
    D_g = F_g;
    D_b = F_b;
  }
  else if (F_a == 0) {
    D_r = B_r;
    D_g = B_g;
    D_b = B_b;
  }
  else {
    D_r = B_r + (F_r-B_r) * opacity / 255;
    D_g = B_g + (F_g-B_g) * opacity / 255;
    D_b = B_b + (F_b-B_b) * opacity / 255;
  }
  D_a = B_a + (F_a-B_a) * opacity / 255;

  return rgba(D_r, D_g, D_b, D_a);
}

// Part of this code comes from pixman library
// Copyright (C) 2000 Keith Packard, member of The XFree86 Project, Inc.
//               2005 Lars Knoll & Zack Rusin, Trolltech

#define ONE_HALF 0x80
#define G_SHIFT 8
#define DIV_ONE_UN8(x) \
    (((x) + ONE_HALF + (((x) + ONE_HALF) >> G_SHIFT)) >> G_SHIFT)

static inline uint32_t
blend_overlay(uint32_t d, uint32_t ad, uint32_t s, uint32_t as)
{
  uint32_t r;

  if (2 * d < ad)
    r = 2 * s * d;
  else
    r = as * ad - 2 * (ad - d) * (as - s);

  return DIV_ONE_UN8(r);
}

int rgba_blend_color_tint(int back, int front, int opacity, int color)
{
  int F_r, F_g, F_b, F_a;
  int B_r, B_g, B_b, B_a;

  B_r = rgba_getr(front);
  B_g = rgba_getg(front);
  B_b = rgba_getb(front);
  B_a = rgba_geta(front);

  F_r = rgba_getr(color);
  F_g = rgba_getg(color);
  F_b = rgba_getb(color);
  F_a = rgba_geta(color);

  F_r = blend_overlay(B_r, B_a, F_r, F_a);
  F_g = blend_overlay(B_g, B_a, F_g, F_a);
  F_b = blend_overlay(B_b, B_a, F_b, F_a);

  F_a = (B_a * (~B_a) + F_a * (~F_a)) / 255;
  F_a += DIV_ONE_UN8(F_a * B_a);

  return rgba_blend_normal(back, rgba(F_r, F_g, F_b, F_a), opacity);
}

int rgba_blend_red_tint(int back, int front, int opacity)
{
  return rgba_blend_color_tint(back, front, opacity, rgba(255, 0, 0, 128));
}

int rgba_blend_blue_tint(int back, int front, int opacity)
{
  return rgba_blend_color_tint(back, front, opacity, rgba(0, 0, 255, 128));
}

int rgba_blend_blackandwhite(int back, int front, int opacity)
{
  int B_r, B_g, B_b, B_a;
  int D_v;

  B_r = rgba_getr(back);
  B_g = rgba_getg(back);
  B_b = rgba_getb(back);
  B_a = rgba_geta(back);

  if ((B_r*30 + B_g*59 + B_b*11)/100 < 128)
    D_v = 255;
  else
    D_v = 0;

  return rgba(D_v, D_v, D_v, B_a);
}

/**********************************************************************/
/* Grayscale blenders                                                 */
/**********************************************************************/

int graya_blend_normal(int back, int front, int opacity)
{
  int t;

  if ((back & 0xff00) == 0) {
    return
      (front & 0xff) |
      (INT_MULT(graya_geta (front), opacity, t) << graya_a_shift);
  }
  else if ((front & 0xff00) == 0) {
    return back;
  }
  else {
    int B_g, B_a;
    int F_g, F_a;
    int D_g, D_a;

    B_g = graya_getv(back);
    B_a = graya_geta(back);

    F_g = graya_getv(front);
    F_a = graya_geta(front);
    F_a = INT_MULT(F_a, opacity, t);

    D_a = B_a + F_a - INT_MULT(B_a, F_a, t);
    D_g = B_g + (F_g-B_g) * F_a / D_a;

    return graya(D_g, D_a);
  }
}

int graya_blend_copy(int back, int front, int opacity)
{
  return front;
}

int graya_blend_forpath(int back, int front, int opacity)
{
  int t, F_k, F_a;

  F_k = graya_getv(front);
  F_a = graya_geta(front);
  F_a = INT_MULT(F_a, opacity, t);

  return graya(F_k, F_a);
}

int graya_blend_merge(int back, int front, int opacity)
{
  int B_k, B_a;
  int F_k, F_a;
  int D_k, D_a;

  B_k = graya_getv(back);
  B_a = graya_geta(back);

  F_k = graya_getv(front);
  F_a = graya_geta(front);

  if (B_a == 0) {
    D_k = F_k;
  }
  else if (F_a == 0) {
    D_k = B_k;
  }
  else {
    D_k = B_k + (F_k-B_k) * opacity / 255;
  }
  D_a = B_a + (F_a-B_a) * opacity / 255;

  return graya(D_k, D_a);
}

int graya_blend_blackandwhite(int back, int front, int opacity)
{
  int B_k, B_a;
  int D_k;

  B_k = graya_getv(back);
  B_a = graya_geta(back);

  if (B_k < 128)
    D_k = 255;
  else
    D_k = 0;

  return graya(D_k, B_a);
}

} // namespace raster
