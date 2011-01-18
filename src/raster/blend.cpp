/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#include "config.h"

#include "raster/blend.h"
#include "raster/image.h"

BLEND_COLOR _rgba_blenders[] =
{
  _rgba_blend_normal,
  _rgba_blend_copy,
};

BLEND_COLOR _graya_blenders[] =
{
  _graya_blend_normal,
  _graya_blend_copy,
};

/**********************************************************************/
/* RGB blenders                                                       */
/**********************************************************************/

int _rgba_blend_normal(int back, int front, int opacity)
{
  register int t;

  if ((back & 0xff000000) == 0) {
    return
      (front & 0xffffff) |
      (INT_MULT(_rgba_geta(front), opacity, t) << _rgba_a_shift);
  }
  else if ((front & 0xff000000) == 0) {
    return back;
  }
  else {
    int B_r, B_g, B_b, B_a;
    int F_r, F_g, F_b, F_a;
    int D_r, D_g, D_b, D_a;

    B_r = _rgba_getr(back);
    B_g = _rgba_getg(back);
    B_b = _rgba_getb(back);
    B_a = _rgba_geta(back);

    F_r = _rgba_getr(front);
    F_g = _rgba_getg(front);
    F_b = _rgba_getb(front);
    F_a = _rgba_geta(front);
    F_a = INT_MULT(F_a, opacity, t);

    D_a = B_a + F_a - INT_MULT(B_a, F_a, t);
    D_r = B_r + (F_r-B_r) * F_a / D_a;
    D_g = B_g + (F_g-B_g) * F_a / D_a;
    D_b = B_b + (F_b-B_b) * F_a / D_a;

    return _rgba(D_r, D_g, D_b, D_a);		
  }						
}

int _rgba_blend_copy(int back, int front, int opacity)
{
  return front;
}

int _rgba_blend_forpath(int back, int front, int opacity)
{
  int F_r, F_g, F_b, F_a;
  register int t;

  F_r = _rgba_getr(front);
  F_g = _rgba_getg(front);
  F_b = _rgba_getb(front);
  F_a = _rgba_geta(front);
  F_a = INT_MULT(F_a, opacity, t);

  return _rgba(F_r, F_g, F_b, F_a);
}

int _rgba_blend_merge(int back, int front, int opacity)
{
  int B_r, B_g, B_b, B_a;
  int F_r, F_g, F_b, F_a;
  int D_r, D_g, D_b, D_a;
    
  B_r = _rgba_getr(back);
  B_g = _rgba_getg(back);
  B_b = _rgba_getb(back);
  B_a = _rgba_geta(back);

  F_r = _rgba_getr(front);
  F_g = _rgba_getg(front);
  F_b = _rgba_getb(front);
  F_a = _rgba_geta(front);

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
  
  return _rgba(D_r, D_g, D_b, D_a);
}

/**********************************************************************/
/* Grayscale blenders                                                 */
/**********************************************************************/

int _graya_blend_normal(int back, int front, int opacity)
{
  register int t;

  if ((back & 0xff00) == 0) {
    return
      (front & 0xff) |
      (INT_MULT(_graya_geta (front), opacity, t) << _graya_a_shift);
  }
  else if ((front & 0xff00) == 0) {
    return back;
  }
  else {
    int B_g, B_a;
    int F_g, F_a;
    int D_g, D_a;

    B_g = _graya_getv(back);
    B_a = _graya_geta(back);

    F_g = _graya_getv(front);
    F_a = _graya_geta(front);
    F_a = INT_MULT(F_a, opacity, t);

    D_a = B_a + F_a - INT_MULT(B_a, F_a, t);
    D_g = B_g + (F_g-B_g) * F_a / D_a;

    return _graya(D_g, D_a);
  }
}

int _graya_blend_copy(int back, int front, int opacity)
{
  return front;
}

int _graya_blend_forpath(int back, int front, int opacity)
{
  int F_k, F_a;
  register int t;

  F_k = _graya_getv(front);
  F_a = _graya_geta(front);
  F_a = INT_MULT(F_a, opacity, t);

  return _graya(F_k, F_a);
}

int _graya_blend_merge(int back, int front, int opacity)
{
  int B_k, B_a;
  int F_k, F_a;
  int D_k, D_a;
    
  B_k = _graya_getv(back);
  B_a = _graya_geta(back);

  F_k = _graya_getv(front);
  F_a = _graya_geta(front);

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
  
  return _graya(D_k, D_a);
}
