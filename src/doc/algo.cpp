// Aseprite Document Library
// Copyright (c) 2001-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/base.h"
#include "doc/algo.h"

#include <algorithm>
#include <cmath>

namespace doc {

void algo_line(int x1, int y1, int x2, int y2, void* data, AlgoPixel proc)
{
  bool yaxis;

  // If the height if the line is bigger than the width, we'll iterate
  // over the y-axis.
  if (ABS(y2-y1) > ABS(x2-x1)) {
    std::swap(x1, y1);
    std::swap(x2, y2);
    yaxis = true;
  }
  else
    yaxis = false;

  const int w = ABS(x2-x1)+1;
  const int h = ABS(y2-y1)+1;
  const int dx = SGN(x2-x1);
  const int dy = SGN(y2-y1);

  int e = 0;
  int y = y1;

  // Move x2 one extra pixel to the dx direction so we can use
  // operator!=() instead of operator<(). Here I prefer operator!=()
  // instead of swapping x1 with x2 so the error always start from 0
  // in the origin (x1,y1).
  x2 += dx;

  for (int x=x1; x!=x2; x+=dx) {
    if (yaxis)
      proc(y, x, data);
    else
      proc(x, y, data);

    // The error advances "h/w" per each "x" step. As we're using a
    // integer value for "e", we use "w" as the unit.
    e += h;
    if (e >= w) {
      y += dy;
      e -= w;
    }
  }
}

/* Additional helper functions for the ellipse-drawing helper function
   below corresponding to routines in Bresenham's algorithm. */
namespace {
  int bresenham_ellipse_error(int rx, int ry, int x, int y)
  {
    return x*x*ry*ry + y*y*rx*rx - rx*rx*ry*ry;
  }

  // Initialize positions x and y for Bresenham's algorithm
  void bresenham_ellipse_init(int rx, int ry, int *px, int *py)
  {
    // Start at the fatter pole
    if (rx > ry) { *px = 0; *py = ry; }
    else { *px = rx; *py = 0; }
  }

  // Move to next pixel to draw, according to Bresenham's algorithm
  void bresenham_ellipse_step(int rx, int ry, int *px, int *py)
  {
    int &x = *px, &y = *py;
    // Move towards the skinnier pole. Having 2 cases isn't needed, but it ensures
    // swapping rx and ry is the same as rotating 90 degrees.
    if (rx > ry) {
      int ex = bresenham_ellipse_error(rx, ry, x, y-1);
      int ey = bresenham_ellipse_error(rx, ry, x+1, y);
      int exy = bresenham_ellipse_error(rx, ry, x+1, y-1);
      if (ex + exy < 0) ++x;
      if (ey + exy > 0) --y;
    }
    else {
      int ex = bresenham_ellipse_error(rx, ry, x, y+1);
      int ey = bresenham_ellipse_error(rx, ry, x-1, y);
      int exy = bresenham_ellipse_error(rx, ry, x-1, y+1);
      if (ex + exy > 0) --x;
      if (ey + exy < 0) ++y;
    }
  }
}

/* Helper function for the ellipse drawing routines. Calculates the
   points of an ellipse which fits onto the rectangle specified by x1,
   y1, x2 and y2, and calls the specified routine for each one. The
   output proc has the same format as for do_line, and if the width or
   height of the ellipse is only 1 or 2 pixels, do_line will be
   called.

   Copyright (C) 2002 by Elias Pschernig (eliaspschernig@aon.at)
   for Allegro 4.x.

   Adapted for ASEPRITE by David A. Capello. */
void algo_ellipse(int x1, int y1, int x2, int y2, void *data, AlgoPixel proc)
{
  int mx, my, rx, ry;
  int x, y;

  /* Cheap hack to get elllipses with integer diameter, by just offsetting
   * some quadrants by one pixel. */
  int mx2, my2;

  mx = (x1 + x2) / 2;
  mx2 = (x1 + x2 + 1) / 2;
  my = (y1 + y2) / 2;
  my2 = (y1 + y2 + 1) / 2;
  rx = ABS(x1 - x2);
  ry = ABS(y1 - y2);

  if (rx == 1) { algo_line(x2, y1, x2, y2, data, proc); rx--; }
  if (rx == 0) { algo_line(x1, y1, x1, y2, data, proc); return; }

  if (ry == 1) { algo_line(x1, y2, x2, y2, data, proc); ry--; }
  if (ry == 0) { algo_line(x1, y1, x2, y1, data, proc); return; }

  rx /= 2;
  ry /= 2;

  /* Draw the 4 poles. */
  proc(mx, my2 + ry, data);
  proc(mx, my - ry, data);
  proc(mx2 + rx, my, data);
  proc(mx - rx, my, data);

  /* For even diameter axis, double the poles. */
  if (mx != mx2) {
    proc(mx2, my2 + ry, data);
    proc(mx2, my - ry, data);
  }

  if (my != my2) {
    proc(mx2 + rx, my2, data);
    proc(mx - rx, my2, data);
  }

  /* Initialize drawing position at a pole. */
  bresenham_ellipse_init(rx, ry, &x, &y);

  for (;;) {
    /* Step to the next pixel to draw. */
    bresenham_ellipse_step(rx, ry, &x, &y);

    /* Edge conditions */
    if (y == 0 && x < rx) ++y; // don't move to horizontal radius except at pole
    if (x == 0 && y < ry) ++x; // don't move to vertical radius except at pole
    if (y <= 0 || x <= 0) break; // stop before pole, since it's already drawn

    /* Process pixel */
    proc(mx2 + x, my - y, data);
    proc(mx - x, my - y, data);
    proc(mx2 + x, my2 + y, data);
    proc(mx - x, my2 + y, data);
  }
}

void algo_ellipsefill(int x1, int y1, int x2, int y2, void *data, AlgoHLine proc)
{
  int mx, my, rx, ry;
  int x, y;

  /* Cheap hack to get elllipses with integer diameter, by just offsetting
   * some quadrants by one pixel. */
  int mx2, my2;

  mx = (x1 + x2) / 2;
  mx2 = (x1 + x2 + 1) / 2;
  my = (y1 + y2) / 2;
  my2 = (y1 + y2 + 1) / 2;
  rx = ABS (x1 - x2);
  ry = ABS (y1 - y2);

  if (rx == 1) { int c; for (c=y1; c<=y2; c++) proc(x2, c, x2, data); rx--; }
  if (rx == 0) { int c; for (c=y1; c<=y2; c++) proc(x1, c, x1, data); return; }

  if (ry == 1) { proc(x1, y2, x2, data); ry--; }
  if (ry == 0) { proc(x1, y1, x2, data); return; }

  rx /= 2;
  ry /= 2;

  /* Draw the north and south poles (possibly 2 pixels) */
  proc(mx, my2 + ry, mx2, data);
  proc(mx, my - ry, mx2, data);

  /* Draw the equator (possibly width 2) */
  proc(mx - rx, my, mx2 + rx, data);
  if (my != my2) proc(mx - rx, my2, mx2 + rx, data);

  /* Initialize drawing position at a pole. */
  bresenham_ellipse_init(rx, ry, &x, &y);

  for (;;) {
    /* Step to the next pixel to draw. */
    bresenham_ellipse_step(rx, ry, &x, &y);

    /* Edge conditions */
    if (y == 0 && x < rx) ++y; // don't move to horizontal radius except at pole
    if (x == 0 && y < ry) ++x; // don't move to vertical radius except at pole
    if (y <= 0 || x <= 0) break; // stop before pole, since it's already drawn

    /* Draw the north and south 'lines of latitude' */
    proc(mx - x, my - y, mx2 + x, data);
    proc(mx - x, my2 + y, mx2 + x, data);
  }
}

// Algorightm from Allegro (allegro/src/spline.c)
// Adapted for Aseprite by David Capello.
void algo_spline(double x0, double y0, double x1, double y1,
                 double x2, double y2, double x3, double y3,
                 void *data, AlgoLine proc)
{
  int npts;
  int out_x1, out_x2;
  int out_y1, out_y2;

  /* Derivatives of x(t) and y(t). */
  double x, dx, ddx, dddx;
  double y, dy, ddy, dddy;
  int i;

  /* Temp variables used in the setup. */
  double dt, dt2, dt3;
  double xdt2_term, xdt3_term;
  double ydt2_term, ydt3_term;

#define MAX_POINTS   64
#undef DIST
#define DIST(x, y) (sqrt((x) * (x) + (y) * (y)))
  npts = (int)(sqrt(DIST(x1-x0, y1-y0) +
                    DIST(x2-x1, y2-y1) +
                    DIST(x3-x2, y3-y2)) * 1.2);
  if (npts > MAX_POINTS)
    npts = MAX_POINTS;
  else if (npts < 4)
    npts = 4;

  dt = 1.0 / (npts-1);
  dt2 = (dt * dt);
  dt3 = (dt2 * dt);

  xdt2_term = 3 * (x2 - 2*x1 + x0);
  ydt2_term = 3 * (y2 - 2*y1 + y0);
  xdt3_term = x3 + 3 * (-x2 + x1) - x0;
  ydt3_term = y3 + 3 * (-y2 + y1) - y0;

  xdt2_term = dt2 * xdt2_term;
  ydt2_term = dt2 * ydt2_term;
  xdt3_term = dt3 * xdt3_term;
  ydt3_term = dt3 * ydt3_term;

  dddx = 6*xdt3_term;
  dddy = 6*ydt3_term;
  ddx = -6*xdt3_term + 2*xdt2_term;
  ddy = -6*ydt3_term + 2*ydt2_term;
  dx = xdt3_term - xdt2_term + 3 * dt * (x1 - x0);
  dy = ydt3_term - ydt2_term + dt * 3 * (y1 - y0);
  x = x0;
  y = y0;

  out_x1 = (int)x0;
  out_y1 = (int)y0;

  x += .5;
  y += .5;
  for (i=1; i<npts; i++) {
    ddx += dddx;
    ddy += dddy;
    dx += ddx;
    dy += ddy;
    x += dx;
    y += dy;

    out_x2 = (int)x;
    out_y2 = (int)y;

    proc(out_x1, out_y1, out_x2, out_y2, data);

    out_x1 = out_x2;
    out_y1 = out_y2;
  }
}

double algo_spline_get_y(double x0, double y0, double x1, double y1,
                         double x2, double y2, double x3, double y3,
                         double in_x)
{
  int npts;
  double out_x, old_x;
  double out_y, old_y;

  /* Derivatives of x(t) and y(t). */
  double x, dx, ddx, dddx;
  double y, dy, ddy, dddy;
  int i;

  /* Temp variables used in the setup. */
  double dt, dt2, dt3;
  double xdt2_term, xdt3_term;
  double ydt2_term, ydt3_term;

#define MAX_POINTS   64
#undef DIST
#define DIST(x, y) (sqrt ((x) * (x) + (y) * (y)))
  npts = (int) (sqrt (DIST(x1-x0, y1-y0) +
                      DIST(x2-x1, y2-y1) +
                      DIST(x3-x2, y3-y2)) * 1.2);
  if (npts > MAX_POINTS)
    npts = MAX_POINTS;
  else if (npts < 4)
    npts = 4;

  dt = 1.0 / (npts-1);
  dt2 = (dt * dt);
  dt3 = (dt2 * dt);

  xdt2_term = 3 * (x2 - 2*x1 + x0);
  ydt2_term = 3 * (y2 - 2*y1 + y0);
  xdt3_term = x3 + 3 * (-x2 + x1) - x0;
  ydt3_term = y3 + 3 * (-y2 + y1) - y0;

  xdt2_term = dt2 * xdt2_term;
  ydt2_term = dt2 * ydt2_term;
  xdt3_term = dt3 * xdt3_term;
  ydt3_term = dt3 * ydt3_term;

  dddx = 6*xdt3_term;
  dddy = 6*ydt3_term;
  ddx = -6*xdt3_term + 2*xdt2_term;
  ddy = -6*ydt3_term + 2*ydt2_term;
  dx = xdt3_term - xdt2_term + 3 * dt * (x1 - x0);
  dy = ydt3_term - ydt2_term + dt * 3 * (y1 - y0);
  x = x0;
  y = y0;

  out_x = old_x = x0;
  out_y = old_y = y0;

  x += .5;
  y += .5;
  for (i=1; i<npts; i++) {
    ddx += dddx;
    ddy += dddy;
    dx += ddx;
    dy += ddy;
    x += dx;
    y += dy;

    out_x = x;
    out_y = y;
    if (out_x > in_x) {
      out_y = old_y + (out_y-old_y) * (in_x-old_x) / (out_x-old_x);
      break;
    }
    old_x = out_x;
    old_y = out_y;
  }

  return out_y;
}

double algo_spline_get_tan(double x0, double y0, double x1, double y1,
                           double x2, double y2, double x3, double y3,
                           double in_x)
{
  double out_x, old_x, old_dx, old_dy;
  int npts;

  /* Derivatives of x(t) and y(t). */
  double x, dx, ddx, dddx;
  double y, dy, ddy, dddy;
  int i;

  /* Temp variables used in the setup. */
  double dt, dt2, dt3;
  double xdt2_term, xdt3_term;
  double ydt2_term, ydt3_term;

#define MAX_POINTS   64
#undef DIST
#define DIST(x, y) (sqrt ((x) * (x) + (y) * (y)))
  npts = (int) (sqrt (DIST(x1-x0, y1-y0) +
                      DIST(x2-x1, y2-y1) +
                      DIST(x3-x2, y3-y2)) * 1.2);
  if (npts > MAX_POINTS)
    npts = MAX_POINTS;
  else if (npts < 4)
    npts = 4;

  dt = 1.0 / (npts-1);
  dt2 = (dt * dt);
  dt3 = (dt2 * dt);

  xdt2_term = 3 * (x2 - 2*x1 + x0);
  ydt2_term = 3 * (y2 - 2*y1 + y0);
  xdt3_term = x3 + 3 * (-x2 + x1) - x0;
  ydt3_term = y3 + 3 * (-y2 + y1) - y0;

  xdt2_term = dt2 * xdt2_term;
  ydt2_term = dt2 * ydt2_term;
  xdt3_term = dt3 * xdt3_term;
  ydt3_term = dt3 * ydt3_term;

  dddx = 6*xdt3_term;
  dddy = 6*ydt3_term;
  ddx = -6*xdt3_term + 2*xdt2_term;
  ddy = -6*ydt3_term + 2*ydt2_term;
  dx = xdt3_term - xdt2_term + 3 * dt * (x1 - x0);
  dy = ydt3_term - ydt2_term + dt * 3 * (y1 - y0);
  x = x0;
  y = y0;

  out_x = x0;

  old_x = x0;
  old_dx = dx;
  old_dy = dy;

  x += .5;
  y += .5;
  for (i=1; i<npts; i++) {
    ddx += dddx;
    ddy += dddy;
    dx += ddx;
    dy += ddy;
    x += dx;
    y += dy;

    out_x = x;
    if (out_x > in_x) {
      dx = old_dx + (dx-old_dx) * (in_x-old_x) / (out_x-old_x);
      dy = old_dy + (dy-old_dy) * (in_x-old_x) / (out_x-old_x);
      break;
    }
    old_x = out_x;
    old_dx = dx;
    old_dy = dy;
  }

  return dy / dx;
}

} // namespace doc
