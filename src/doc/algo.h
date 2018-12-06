// Aseprite Document Library
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (c) 2001-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_ALGO_H_INCLUDED
#define DOC_ALGO_H_INCLUDED
#pragma once

#include "gfx/fwd.h"
#include "doc/algorithm/hline.h"

namespace doc {

  class Image;

  typedef void (*AlgoPixel)(int x, int y, void *data);
  typedef void (*AlgoLine)(int x1, int y1, int x2, int y2, void *data);

  // Useful to create lines with more predictable behavior and perfect
  // pixel block of lines where we'll have a number of lines/rows that
  // looks the same.
  //
  // Related to: https://github.com/aseprite/aseprite/issues/1395
  void algo_line_perfect(int x1, int y1, int x2, int y2, void* data, AlgoPixel proc);

  // Useful to create continuous lines (you can draw from one point to
  // another, and continue from that point to another in the same
  // angle and the line will look continous).
  //
  // Related to:
  // https://community.aseprite.org/t/1045
  // https://github.com/aseprite/aseprite/issues/1894
  void algo_line_continuous(int x1, int y1, int x2, int y2, void *data, AlgoPixel proc);

  void algo_ellipse(int x1, int y1, int x2, int y2, void *data, AlgoPixel proc);
  void algo_ellipsefill(int x1, int y1, int x2, int y2, void *data, AlgoHLine proc);

  void draw_rotated_ellipse(int cx, int cy, int a, int b, double angle, void* data, AlgoPixel proc);
  void fill_rotated_ellipse(int cx, int cy, int a, int b, double angle, void* data, AlgoHLine proc);

  void algo_spline(double x0, double y0, double x1, double y1,
                   double x2, double y2, double x3, double y3,
                   void *data, AlgoLine proc);
  double algo_spline_get_y(double x0, double y0, double x1, double y1,
                           double x2, double y2, double x3, double y3,
                           double x);
  double algo_spline_get_tan(double x0, double y0, double x1, double y1,
                             double x2, double y2, double x3, double y3,
                             double in_x);

} // namespace doc

#endif
