// Aseprite Document Library
// Copyright (c) 2018-2020 Igara Studio S.A.
// Copyright (c) 2001-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/algo.h"

#include "base/clamp.h"
#include "base/debug.h"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace doc {

void algo_line_perfect(int x1, int y1, int x2, int y2, void* data, AlgoPixel proc)
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

// Special version of the perfect line algorithm specially done for
// kLineBrushType so the whole line looks continuous without holes.
//
// TOOD in a future we should convert lines into scanlines and render
//      scanlines instead of drawing the brush on each pixel, that
//      would fix all cases
void algo_line_perfect_with_fix_for_line_brush(int x1, int y1, int x2, int y2, void* data, AlgoPixel proc)
{
  bool yaxis;

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

  x2 += dx;

  for (int x=x1; x!=x2; x+=dx) {
    if (yaxis)
      proc(y, x, data);
    else
      proc(x, y, data);

    e += h;
    if (e >= w) {
      y += dy;
      e -= w;
      if (x+dx != x2) {
        if (yaxis)
          proc(y, x, data);
        else
          proc(x, y, data);
      }
    }
  }
}

// Line code based on Alois Zingl work released under the
// MIT license http://members.chello.at/easyfilter/bresenham.html
void algo_line_continuous(int x0, int y0, int x1, int y1, void* data, AlgoPixel proc)
{
  int dx =  ABS(x1-x0), sx = (x0 < x1 ? 1: -1);
  int dy = -ABS(y1-y0), sy = (y0 < y1 ? 1: -1);
  int err = dx+dy, e2;                                  // error value e_xy

  for (;;) {
    proc(x0, y0, data);
    e2 = 2*err;
    if (e2 >= dy) {                                       // e_xy+e_x > 0
      if (x0 == x1)
        break;
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {                                       // e_xy+e_y < 0
      if (y0 == y1)
        break;
      err += dx;
      y0 += sy;
    }
  }
}

// Special version of the continuous line algorithm specially done for
// kLineBrushType so the whole line looks continuous without holes.
void algo_line_continuous_with_fix_for_line_brush(int x0, int y0, int x1, int y1, void* data, AlgoPixel proc)
{
  int dx =  ABS(x1-x0), sx = (x0 < x1 ? 1: -1);
  int dy = -ABS(y1-y0), sy = (y0 < y1 ? 1: -1);
  int err = dx+dy, e2;                                  // error value e_xy
  bool x_changed;

  for (;;) {
    x_changed = false;

    proc(x0, y0, data);
    e2 = 2*err;
    if (e2 >= dy) {                                       // e_xy+e_x > 0
      if (x0 == x1)
        break;
      err += dy;
      x0 += sx;
      x_changed = true;
    }
    if (e2 <= dx) {                                       // e_xy+e_y < 0
      if (y0 == y1)
        break;
      err += dx;
      if (x_changed)
        proc(x0, y0, data);
      y0 += sy;
    }
  }
}

// Ellipse code based on Alois Zingl work released under the MIT
// license http://members.chello.at/easyfilter/bresenham.html
//
// Adapted for Aseprite by David Capello

void algo_ellipse(int x0, int y0, int x1, int y1, void* data, AlgoPixel proc)
{
  long a = abs(x1-x0), b = abs(y1-y0), b1 = b&1;                 // diameter
  double dx = 4*(1.0-a)*b*b, dy = 4*(b1+1)*a*a;           // error increment
  double err = dx+dy+b1*a*a, e2;                          // error of 1.step

  if (x0 > x1) { x0 = x1; x1 += a; }        // if called with swapped points
  if (y0 > y1) y0 = y1;                                  // .. exchange them
  y0 += (b+1)/2; y1 = y0-b1;                               // starting pixel
  a = 8*a*a; b1 = 8*b*b;

  do {
    proc(x1, y0, data);                                      //   I. Quadrant
    proc(x0, y0, data);                                      //  II. Quadrant
    proc(x0, y1, data);                                      // III. Quadrant
    proc(x1, y1, data);                                      //  IV. Quadrant
    e2 = 2*err;
    if (e2 <= dy) { y0++; y1--; err += dy += a; }                 // y step
    if (e2 >= dx || 2*err > dy) { x0++; x1--; err += dx += b1; }  // x step
  } while (x0 <= x1);

  while (y0-y1 <= b) {          // too early stop of flat ellipses a=1
    proc(x0-1, y0, data);       // -> finish tip of ellipse
    proc(x1+1, y0++, data);
    proc(x0-1, y1, data);
    proc(x1+1, y1--, data);
  }
}

void algo_ellipsefill(int x0, int y0, int x1, int y1, void* data, AlgoHLine proc)
{
  long a = abs(x1-x0), b = abs(y1-y0), b1 = b&1;                 // diameter
  double dx = 4*(1.0-a)*b*b, dy = 4*(b1+1)*a*a;           // error increment
  double err = dx+dy+b1*a*a, e2;                          // error of 1.step

  if (x0 > x1) { x0 = x1; x1 += a; }        // if called with swapped points
  if (y0 > y1) y0 = y1;                                  // .. exchange them
  y0 += (b+1)/2; y1 = y0-b1;                               // starting pixel
  a = 8*a*a; b1 = 8*b*b;

  do {
    proc(x0, y0, x1, data);
    proc(x0, y1, x1, data);
    e2 = 2*err;
    if (e2 <= dy) { y0++; y1--; err += dy += a; }                 // y step
    if (e2 >= dx || 2*err > dy) { x0++; x1--; err += dx += b1; }  // x step
  } while (x0 <= x1);

  while (y0-y1 <= b) {          // too early stop of flat ellipses a=1
    proc(x0-1, y0, x0-1, data);       // -> finish tip of ellipse
    proc(x1+1, y0++, x1+1, data);
    proc(x0-1, y1, x0-1, data);
    proc(x1+1, y1--, x1+1, data);
  }
}

static void draw_quad_rational_bezier_seg(int x0, int y0,
                                          int x1, int y1,
                                          int x2, int y2,
                                          double w,
                                          void* data,
                                          AlgoPixel proc)
{                   // plot a limited rational Bezier segment, squared weight
  int sx = x2-x1;                 // relative values for checks
  int sy = y2-y1;
  int dx = x0-x2;
  int dy = y0-y2;
  int xx = x0-x1;
  int yy = y0-y1;
  double xy = xx*sy + yy*sx;
  double cur = xx*sy - yy*sx;   // curvature
  double err;

  ASSERT(xx*sx <= 0.0 && yy*sy <= 0.0);   // sign of gradient must not change

  if (cur != 0.0 && w > 0.0) {                            // no straight line
    if (sx*sx+sy*sy > xx*xx+yy*yy) {               // begin with shorter part
      // swap P0 P2
      x2 = x0;
      x0 -= dx;
      y2 = y0;
      y0 -= dy;
      cur = -cur;
    }
    xx = 2.0*(4.0*w*sx*xx+dx*dx);                   // differences 2nd degree
    yy = 2.0*(4.0*w*sy*yy+dy*dy);
    sx = x0 < x2 ? 1 : -1;                                // x step direction
    sy = y0 < y2 ? 1 : -1;                                // y step direction
    xy = -2.0*sx*sy*(2.0*w*xy+dx*dy);

    if (cur*sx*sy < 0.0) {                              // negated curvature?
      xx = -xx; yy = -yy; xy = -xy; cur = -cur;
    }
    dx = 4.0*w*(x1-x0)*sy*cur+xx/2.0+xy;            // differences 1st degree
    dy = 4.0*w*(y0-y1)*sx*cur+yy/2.0+xy;

    if (w < 0.5 && (dy > xy || dx < xy)) {   // flat ellipse, algorithm fails
      cur = (w+1.0)/2.0;
      w = std::sqrt(w);
      xy = 1.0/(w+1.0);

      sx = std::floor((x0+2.0*w*x1+x2)*xy/2.0+0.5); // subdivide curve in half
      sy = std::floor((y0+2.0*w*y1+y2)*xy/2.0+0.5);

      dx = std::floor((w*x1+x0)*xy+0.5);
      dy = std::floor((y1*w+y0)*xy+0.5);
      draw_quad_rational_bezier_seg(x0, y0, dx, dy, sx, sy, cur, data, proc); // plot separately

      dx = std::floor((w*x1+x2)*xy+0.5);
      dy = std::floor((y1*w+y2)*xy+0.5);
      draw_quad_rational_bezier_seg(sx, sy, dx, dy, x2, y2, cur, data, proc);
      return;
    }
    err = dx+dy-xy;             // error 1.step
    do {
      // plot curve
      proc(x0, y0, data);

      if (x0 == x2 && y0 == y2)
        return;       // last pixel -> curve finished

      x1 = 2*err > dy;
      y1 = 2*(err+yy) < -dy; // save value for test of x step

      if (2*err < dx || y1) {
        // y step
        y0 += sy;
        dy += xy;
        err += dx += xx;
      }
      if (2*err > dx || x1) {
        // x step
        x0 += sx;
        dx += xy;
        err += dy += yy;
      }
    } while (dy <= xy && dx >= xy);    // gradient negates -> algorithm fails
  }
  algo_line_continuous(x0, y0, x2, y2, data, proc); // plot remaining needle to end
}

static void draw_rotated_ellipse_rect(int x0, int y0, int x1, int y1, double zd, void* data, AlgoPixel proc)
{
  int xd = x1-x0;
  int yd = y1-y0;
  double w = xd*yd;

  if (zd == 0)
    return algo_ellipse(x0, y0, x1, y1, data, proc);

  if (w != 0.0)
    w = (w-zd) / (w+w); // squared weight of P1

  w = base::clamp(w, 0.0, 1.0);

  xd = std::floor(w*xd + 0.5);
  yd = std::floor(w*yd + 0.5);

  draw_quad_rational_bezier_seg(x0, y0+yd, x0, y0, x0+xd, y0, 1.0-w, data, proc);
  draw_quad_rational_bezier_seg(x0, y0+yd, x0, y1, x1-xd, y1, w,     data, proc);
  draw_quad_rational_bezier_seg(x1, y1-yd, x1, y1, x1-xd, y1, 1.0-w, data, proc);
  draw_quad_rational_bezier_seg(x1, y1-yd, x1, y0, x0+xd, y0, w,     data, proc);
}

void draw_rotated_ellipse(int cx, int cy, int a, int b, double angle, void* data, AlgoPixel proc)
{
  double xd = a*a;
  double yd = b*b;
  double s = std::sin(angle);
  double zd = (xd-yd)*s;        // ellipse rotation
  xd = std::sqrt(xd-zd*s);      // surrounding rectangle
  yd = std::sqrt(yd+zd*s);

  a = std::floor(xd+0.5);
  b = std::floor(yd+0.5);
  zd = zd*a*b / (xd*yd);
  zd = 4*zd*std::cos(angle);

  draw_rotated_ellipse_rect(cx-a, cy-b, cx+a, cy+b, zd, data, proc);
}

void fill_rotated_ellipse(int cx, int cy, int a, int b, double angle, void* data, AlgoHLine proc)
{
  struct Rows {
    int y0;
    std::vector<std::pair<int, int> > row;
    Rows(int y0, int nrows)
      : y0(y0), row(nrows, std::make_pair(1, -1)) { }
    void update(int x, int y) {
      int i = base::clamp(y-y0, 0, int(row.size()-1));
      auto& r = row[i];
      if (r.first > r.second) {
        r.first = r.second = x;
      }
      else {
        r.first = std::min(r.first, x);
        r.second = std::max(r.second, x);
      }
    }
  };

  double xd = a*a;
  double yd = b*b;
  double s = std::sin(angle);
  double zd = (xd-yd)*s;
  xd = std::sqrt(xd-zd*s);
  yd = std::sqrt(yd+zd*s);

  a = std::floor(xd+0.5);
  b = std::floor(yd+0.5);
  zd = zd*a*b / (xd*yd);
  zd = 4*zd*std::cos(angle);

  Rows rows(cy-b, 2*b+1);

  draw_rotated_ellipse_rect(
    cx-a, cy-b, cx+a, cy+b, zd,
    &rows,
    [](int x, int y, void* data){
      Rows* rows = (Rows*)data;
      rows->update(x, y);
    });

  int y = rows.y0;
  for (const auto& r : rows.row) {
    if (r.first <= r.second)
      proc(r.first, y, r.second, data);
    ++y;
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

  old_x = x0;
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
