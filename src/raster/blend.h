/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#ifndef RASTER_BLEND_H_INCLUDED
#define RASTER_BLEND_H_INCLUDED

#define INT_MULT(a, b, t)				\
  ((t) = (a) * (b) + 0x80, ((((t) >> 8) + (t)) >> 8))

enum {
  BLEND_MODE_NORMAL,
  BLEND_MODE_DISSOLVE,
  BLEND_MODE_MULTIPLY,
  BLEND_MODE_SCREEN,
  BLEND_MODE_OVERLAY,
  BLEND_MODE_HARD_LIGHT,
  BLEND_MODE_DODGE,
  BLEND_MODE_BURN,
  BLEND_MODE_DARKEN,
  BLEND_MODE_LIGHTEN,
  BLEND_MODE_ADDITION,
  BLEND_MODE_SUBTRACT,
  BLEND_MODE_DIFFERENCE,
  BLEND_MODE_HUE,
  BLEND_MODE_SATURATION,
  BLEND_MODE_COLOR,
  BLEND_MODE_LUMINOSITY,
  BLEND_MODE_COPY,
  BLEND_MODE_MAX,
};

typedef int (*BLEND_COLOR)(int back, int front, int opacity);

extern BLEND_COLOR _rgba_blenders[];
extern BLEND_COLOR _graya_blenders[];

int _rgba_blend_normal(int back, int front, int opacity);
int _rgba_blend_dissolve(int back, int front, int opacity);
int _rgba_blend_multiply(int back, int front, int opacity);
int _rgba_blend_screen(int back, int front, int opacity);
int _rgba_blend_overlay(int back, int front, int opacity);
int _rgba_blend_hard_light(int back, int front, int opacity);
int _rgba_blend_dodge(int back, int front, int opacity);
int _rgba_blend_burn(int back, int front, int opacity);
int _rgba_blend_darken(int back, int front, int opacity);
int _rgba_blend_lighten(int back, int front, int opacity);
int _rgba_blend_addition(int back, int front, int opacity);
int _rgba_blend_subtract(int back, int front, int opacity);
int _rgba_blend_difference(int back, int front, int opacity);
int _rgba_blend_hue(int back, int front, int opacity);
int _rgba_blend_saturation(int back, int front, int opacity);
int _rgba_blend_color(int back, int front, int opacity);
int _rgba_blend_luminosity(int back, int front, int opacity);
int _rgba_blend_copy(int back, int front, int opacity);
int _rgba_blend_FORPATH(int back, int front, int opacity);
int _rgba_blend_MERGE(int back, int front, int opacity);

int _graya_blend_normal(int back, int front, int opacity);
int _graya_blend_copy(int back, int front, int opacity);
int _graya_blend_FORPATH(int back, int front, int opacity);
int _graya_blend_MERGE(int back, int front, int opacity);

void rgb_to_hsv_int(int *red, int *green, int *blue);
void hsv_to_rgb_int(int *hue, int *saturation, int *value);

#endif

