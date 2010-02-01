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

#ifndef EFFECT_COLCURVE_H_INCLUDED
#define EFFECT_COLCURVE_H_INCLUDED

#include "jinete/jbase.h"

struct Effect;

enum {
  CURVE_LINEAR,
  CURVE_SPLINE,
};

typedef struct CurvePoint
{
  int x, y;
} CurvePoint;

typedef struct Curve
{
  int type;
  JList points;
} Curve;

CurvePoint *curve_point_new(int x, int y);
void curve_point_free(CurvePoint *point);

Curve *curve_new(int type);
void curve_free(Curve *curve);
void curve_add_point(Curve *curve, CurvePoint *point);
void curve_remove_point(Curve *curve, CurvePoint *point);
void curve_get_values(Curve *curve, int x1, int x2, int *values);

void set_color_curve(Curve *curve);

void apply_color_curve4(struct Effect *effect);
void apply_color_curve2(struct Effect *effect);
void apply_color_curve1(struct Effect *effect);

#endif
