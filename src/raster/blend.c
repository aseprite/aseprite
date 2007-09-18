/* ase -- allegro-sprite-editor: the ultimate sprites factory
 * Copyright (C) 2001-2005, 2007  David A. Capello
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
 *
 ***********************************************************************
 *
 * Various blender's algorithms were "stolen" from The GIMP with 
 * Copyright (C) 1995 of Spencer Kimball and Peter Mattis
 * See the file "gimp/app/paint-funcs/paint-funcs-generic.h".
 */

#include "config.h"

#ifndef USE_PRECOMPILED_HEADER

#include "raster/blend.h"
#include "raster/image.h"

#endif

#ifdef USE_X86_INT_MULT
#  undef INT_MULT(a, b, t)
#  define INT_MULT(a, b, t) (_int_mult (a, b))
#  define T_VAR
#  define NT_VAR register int t;
  extern int _int_mult (int a, int b);
#else
#  define T_VAR register int t;
#  define NT_VAR
#endif

BLEND_COLOR _rgba_blenders[] =
{
  _rgba_blend_normal,
  _rgba_blend_normal, /* _rgba_blend_dissolve, */
  _rgba_blend_multiply,
  _rgba_blend_screen,
  _rgba_blend_overlay,
  _rgba_blend_normal, /* _rgba_blend_hard_light, */
  _rgba_blend_normal, /* _rgba_blend_dodge, */
  _rgba_blend_normal, /* _rgba_blend_burn, */
  _rgba_blend_darken,
  _rgba_blend_lighten,
  _rgba_blend_addition,
  _rgba_blend_subtract,
  _rgba_blend_difference,
  _rgba_blend_hue,
  _rgba_blend_saturation,
  _rgba_blend_color,
  _rgba_blend_luminosity,
  _rgba_blend_copy,
};

BLEND_COLOR _graya_blenders[] =
{
  _graya_blend_normal,
  _graya_blend_normal, /* _graya_blend_dissolve, */
  _graya_blend_normal, /* _graya_blend_multiply, */
  _graya_blend_normal, /* _graya_blend_screen, */
  _graya_blend_normal, /* _graya_blend_overlay, */
  _graya_blend_normal, /* _graya_blend_hard_light, */
  _graya_blend_normal, /* _graya_blend_dodge, */
  _graya_blend_normal, /* _graya_blend_burn, */
  _graya_blend_normal, /* _graya_blend_darken, */
  _graya_blend_normal, /* _graya_blend_lighten, */
  _graya_blend_normal, /* _graya_blend_addition, */
  _graya_blend_normal, /* _graya_blend_subtract, */
  _graya_blend_normal, /* _graya_blend_difference, */
  _graya_blend_normal, /* _graya_blend_hue, */
  _graya_blend_normal, /* _graya_blend_saturation, */
  _graya_blend_normal, /* _graya_blend_color, */
  _graya_blend_normal, /* _graya_blend_luminosity */
  _graya_blend_copy,
};

unsigned char _index_cmap[256] =
{
   0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
  16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
  32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
  48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
  64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
  80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
  96, 97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111,
 112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
 128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
 144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
 160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
 176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
 192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
 208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
 224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
 240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255
};

/**********************************************************************/
/* RGB blenders                                                       */
/**********************************************************************/

#define BLEND_BEGIN(mode)						\
int _rgba_blend_##mode (int back, int front, int opacity)		\
{									\
  T_VAR									\
									\
  if ((back & 0xff000000) == 0) {					\
    return								\
      (front & 0xffffff) |						\
      (INT_MULT(_rgba_geta (front), opacity, t) << _rgba_a_shift);	\
  }									\
  else if ((front & 0xff000000) == 0) {					\
    return back;							\
  }									\
  else {								\
    int B_r, B_g, B_b, B_a;						\
    int F_r, F_g, F_b, F_a;						\
    int D_r, D_g, D_b, D_a;						\
									\
    B_r = _rgba_getr(back);						\
    B_g = _rgba_getg(back);						\
    B_b = _rgba_getb(back);						\
    B_a = _rgba_geta(back);						\
									\
    F_r = _rgba_getr(front);						\
    F_g = _rgba_getg(front);						\
    F_b = _rgba_getb(front);						\
    F_a = _rgba_geta(front);						\
    F_a = INT_MULT(F_a, opacity, t);

#define BLEND_END()				\
    return _rgba(D_r, D_g, D_b, D_a);		\
  }						\
}

#define BLEND_OPACITY()				\
  D_r = B_r + (D_r-B_r) * F_a / 255;		\
  D_g = B_g + (D_g-B_g) * F_a / 255;		\
  D_b = B_b + (D_b-B_b) * F_a / 255;

BLEND_BEGIN(normal)
  D_a = B_a + F_a - INT_MULT(B_a, F_a, t);
  D_r = B_r + (F_r-B_r) * F_a / D_a;
  D_g = B_g + (F_g-B_g) * F_a / D_a;
  D_b = B_b + (F_b-B_b) * F_a / D_a;
BLEND_END()

BLEND_BEGIN(multiply)
  D_r = INT_MULT(B_r, F_r, t);
  D_g = INT_MULT(B_g, F_g, t);
  D_b = INT_MULT(B_b, F_b, t);
  D_a = B_a;

  BLEND_OPACITY()
BLEND_END()

BLEND_BEGIN(screen)
  D_r = 255 - INT_MULT((255 - B_r), (255 - F_r), t);
  D_g = 255 - INT_MULT((255 - B_g), (255 - F_g), t);
  D_b = 255 - INT_MULT((255 - B_b), (255 - F_b), t);
  D_a = B_a;

  BLEND_OPACITY()
BLEND_END()

BLEND_BEGIN(overlay)
  D_r = INT_MULT(B_r, B_r + INT_MULT (2 * F_r, 255 - B_r, t), t);
  D_g = INT_MULT(B_g, B_g + INT_MULT (2 * F_g, 255 - B_g, t), t);
  D_b = INT_MULT(B_b, B_b + INT_MULT (2 * F_b, 255 - B_b, t), t);
  D_a = B_a;

  BLEND_OPACITY()
BLEND_END()

#if 0
BLEND_BEGIN(hard_light)
#define HARD_LIGHT(channel)						\
    if (F_##channel > 128) {						\
      t = (255 - B_##channel) * (255 - ((F_##channel - 128) << 1));	\
      D_##channel = MID (0, 255 - (t >> 8), 255);			\
    }									\
    else {								\
      t = B_##channel * (F_##channel << 1);				\
      D_##channel = MID (0, t >> 8, 255);				\
    }

  HARD_LIGHT(r)
  HARD_LIGHT(g)
  HARD_LIGHT(b)
  D_a = B_a;

  BLEND_OPACITY()
BLEND_END()
#endif

BLEND_BEGIN(darken)
  D_r = MIN(B_r, F_r);
  D_g = MIN(B_g, F_g);
  D_b = MIN(B_b, F_b);
  D_a = B_a;

  BLEND_OPACITY()
BLEND_END()

BLEND_BEGIN(lighten)
  D_r = MAX(B_r, F_r);
  D_g = MAX(B_g, F_g);
  D_b = MAX(B_b, F_b);
  D_a = B_a;

  BLEND_OPACITY()
BLEND_END()

BLEND_BEGIN(addition)
  D_r = MIN(B_r + F_r, 255);
  D_g = MIN(B_g + F_g, 255);
  D_b = MIN(B_b + F_b, 255);
  D_a = B_a;

  BLEND_OPACITY()
BLEND_END()

BLEND_BEGIN(subtract)
  D_r = MAX(B_r - F_r, 0);
  D_g = MAX(B_g - F_g, 0);
  D_b = MAX(B_b - F_b, 0);
  D_a = B_a;

  BLEND_OPACITY()
BLEND_END()

BLEND_BEGIN(difference)
  {
    NT_VAR
    t = B_r - F_r; D_r = (t < 0)? -t: t;
    t = B_g - F_g; D_g = (t < 0)? -t: t;
    t = B_b - F_b; D_b = (t < 0)? -t: t;
  }
  D_a = B_a;

  BLEND_OPACITY()
BLEND_END()

BLEND_BEGIN(hue)
  D_r = B_r;
  D_g = B_g;
  D_b = B_b;
  D_a = B_a;

  rgb_to_hsv_int (&D_r, &D_g, &D_b);
  rgb_to_hsv_int (&F_r, &F_g, &F_b);

  D_r = F_r;

  hsv_to_rgb_int (&D_r, &D_g, &D_b);

  BLEND_OPACITY()
BLEND_END()

BLEND_BEGIN(saturation)
  D_r = B_r;
  D_g = B_g;
  D_b = B_b;
  D_a = B_a;

  rgb_to_hsv_int (&D_r, &D_g, &D_b);
  rgb_to_hsv_int (&F_r, &F_g, &F_b);

  D_g = F_g;

  hsv_to_rgb_int (&D_r, &D_g, &D_b);

  BLEND_OPACITY()
BLEND_END()

BLEND_BEGIN(color)
  D_r = B_r;
  D_g = B_g;
  D_b = B_b;
  D_a = B_a;

  rgb_to_hsv_int (&D_r, &D_g, &D_b);
  rgb_to_hsv_int (&F_r, &F_g, &F_b);

  D_r = F_r;
  D_g = F_g;

  hsv_to_rgb_int (&D_r, &D_g, &D_b);

  BLEND_OPACITY()
BLEND_END()

BLEND_BEGIN(luminosity)
  D_r = B_r;
  D_g = B_g;
  D_b = B_b;
  D_a = B_a;

  rgb_to_hsv_int (&D_r, &D_g, &D_b);
  rgb_to_hsv_int (&F_r, &F_g, &F_b);

  D_b = F_b;

  hsv_to_rgb_int (&D_r, &D_g, &D_b);

  BLEND_OPACITY()
BLEND_END()

int _rgba_blend_copy(int back, int front, int opacity)
{
  return front;
}

int _rgba_blend_FORPATH(int back, int front, int opacity)
{
  int F_r, F_g, F_b, F_a;
  T_VAR

  F_r = _rgba_getr(front);
  F_g = _rgba_getg(front);
  F_b = _rgba_getb(front);
  F_a = _rgba_geta(front);
  F_a = INT_MULT(F_a, opacity, t);

  return _rgba(F_r, F_g, F_b, F_a);
}

#undef BLEND_BEGIN
#undef BLEND_END
#undef BLEND_OPACITY

/**********************************************************************/
/* Grayscale blenders                                                 */
/**********************************************************************/

#define BLEND_BEGIN(mode)						\
int _graya_blend_##mode (int back, int front, int opacity)		\
{									\
  T_VAR									\
									\
  if ((back & 0xff00) == 0) {						\
    return								\
      (front & 0xff) |							\
      (INT_MULT(_graya_geta (front), opacity, t) << _graya_a_shift);	\
  }									\
  else if ((front & 0xff00) == 0) {					\
    return back;							\
  }									\
  else {								\
    int B_g, B_a;							\
    int F_g, F_a;							\
    int D_g, D_a;							\
									\
    B_g = _graya_getk(back);						\
    B_a = _graya_geta(back);						\
									\
    F_g = _graya_getk(front);						\
    F_a = _graya_geta(front);						\
    F_a = INT_MULT(F_a, opacity, t);

#define BLEND_END()				\
    return _graya(D_g, D_a);			\
  }						\
}

#define BLEND_OPACITY()				\
  D_g = B_g + (D_g-B_g) * F_a / 255;

BLEND_BEGIN(normal)
  D_a = B_a + F_a - INT_MULT(B_a, F_a, t);
  D_g = B_g + (F_g-B_g) * F_a / D_a;
BLEND_END()

int _graya_blend_copy(int back, int front, int opacity)
{
  return front;
}

/* BLEND_BEGIN(FORPATH) */
/*   D_a = B_a + F_a - INT_MULT(B_a, F_a, t); */
/*   D_g = B_g + (F_g-B_g) * F_a / D_a; */
/* BLEND_END() */
int _graya_blend_FORPATH(int back, int front, int opacity)
{
  int F_k, F_a;
  T_VAR

  F_k = _graya_getk(front);
  F_a = _graya_geta(front);
  F_a = INT_MULT(F_a, opacity, t);

  return _graya(F_k, F_a);
}

/**********************************************************************/
/* Routines from The GIMP                                             */
/**********************************************************************/

void rgb_to_hsv_int(int *red, int *green, int *blue)
{
  int    r, g, b;
  double h, s, v;
  int    min, max;
  int    delta;

  h = 0.0;

  r = *red;
  g = *green;
  b = *blue;

  if (r > g)
    {
      max = MAX (r, b);
      min = MIN (g, b);
    }
  else
    {
      max = MAX (g, b);
      min = MIN (r, b);
    }

  v = max;

  if (max != 0)
    s = ((max - min) * 255) / (double) max;
  else
    s = 0;

  if (s == 0)
    h = 0;
  else
    {
      delta = max - min;
      if (r == max)
	h = (g - b) / (double) delta;
      else if (g == max)
	h = 2 + (b - r) / (double) delta;
      else if (b == max)
	h = 4 + (r - g) / (double) delta;
      h *= 42.5;

      if (h < 0)
	h += 255;
      if (h > 255)
	h -= 255;
    }

  *red   = (int)h;
  *green = (int)s;
  *blue  = (int)v;
}

void hsv_to_rgb_int(int *hue, int *saturation, int *value)
{
  double h, s, v;
  double f, p, q, t;

  if (*saturation == 0)
    {
      *hue        = *value;
      *saturation = *value;
      *value      = *value;
    }
  else
    {
      h = *hue * 6.0  / 255.0;
      s = *saturation / 255.0;
      v = *value      / 255.0;

      f = h - (int) h;
      p = v * (1.0 - s);
      q = v * (1.0 - (s * f));
      t = v * (1.0 - (s * (1.0 - f)));

      switch ((int) h)
	{
	case 0:
	  *hue        = (int)(v * 255);
	  *saturation = (int)(t * 255);
	  *value      = (int)(p * 255);
	  break;

	case 1:
	  *hue        = (int)(q * 255);
	  *saturation = (int)(v * 255);
	  *value      = (int)(p * 255);
	  break;

	case 2:
	  *hue        = (int)(p * 255);
	  *saturation = (int)(v * 255);
	  *value      = (int)(t * 255);
	  break;

	case 3:
	  *hue        = (int)(p * 255);
	  *saturation = (int)(q * 255);
	  *value      = (int)(v * 255);
	  break;

	case 4:
	  *hue        = (int)(t * 255);
	  *saturation = (int)(p * 255);
	  *value      = (int)(v * 255);
	  break;

	case 5:
	  *hue        = (int)(v * 255);
	  *saturation = (int)(p * 255);
	  *value      = (int)(q * 255);
	  break;
	}
    }
}
