// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.
//
// TODO rewrite this algorithm from scratch

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/algorithm/polygon.h"

#include "gfx/point.h"

#include <vector>

namespace doc {

using namespace gfx;

// polygon() is an adaptation from Matthieu Haller code of GD library

// THANKS to Kirsten Schulz for the polygon fixes!

/* The intersection finding technique of this code could be improved  */
/* by remembering the previous intertersection, and by using the slope. */
/* That could help to adjust intersections  to produce a nice */
/* interior_extrema. */

void algorithm::polygon(int vertices, const int* points, void* data, AlgoHLine proc)
{
  int n = vertices;
  if (!n)
    return;

  int i;
  int j;
  int index;
  int y;
  int miny, maxy;
  int x1, y1;
  int x2, y2;
  int ind1, ind2;
  int ints;

  std::vector<int> polyInts(n);
  std::vector<Point> p(n);
  for (i = 0; (i < n); i++) {
    p[i].x = points[i*2];
    p[i].y = points[i*2+1];
  }

  miny = p[0].y;
  maxy = p[0].y;
  for (i = 1; (i < n); i++) {
    if (p[i].y < miny) miny = p[i].y;
    if (p[i].y > maxy) maxy = p[i].y;
  }

  /* 2.0.16: Optimization by Ilia Chipitsine -- don't waste time offscreen */
  /* 2.0.26: clipping rectangle is even better */
// if (miny < im->cy1) {
//   miny = im->cy1;
// }
// if (maxy > im->cy2) {
//   maxy = im->cy2;
// }

  /* Fix in 1.3: count a vertex only once */
  for (y = miny; (y <= maxy); y++) {
    /*1.4           int interLast = 0; */
    /*              int dirLast = 0; */
    /*              int interFirst = 1; */
    /* 2.0.26+      int yshift = 0; */

    ints = 0;
    for (i = 0; (i < n); i++) {
      if (!i) {
        ind1 = n - 1;
        ind2 = 0;
      }
      else {
        ind1 = i - 1;
        ind2 = i;
      }
      y1 = p[ind1].y;
      y2 = p[ind2].y;
      if (y1 < y2) {
        x1 = p[ind1].x;
        x2 = p[ind2].x;
      }
      else if (y1 > y2) {
        y2 = p[ind1].y;
        y1 = p[ind2].y;
        x2 = p[ind1].x;
        x1 = p[ind2].x;
      }
      else {
        continue;
      }

      /* Do the following math as float intermediately, and round to ensure
       * that Polygon and FilledPolygon for the same set of points have the
       * same footprint. */

      if ((y >= y1) && (y < y2)) {
        polyInts[ints++] = (int) ((float) ((y - y1) * (x2 - x1)) /
                                  (float) (y2 - y1) + 0.5 + x1);
      }
      else if ((y == maxy) && (y > y1) && (y <= y2)) {
        polyInts[ints++] = (int) ((float) ((y - y1) * (x2 - x1)) /
                                  (float) (y2 - y1) + 0.5 + x1);
      }
    }
    /*
       2.0.26: polygons pretty much always have less than 100 points,
       and most of the time they have considerably less. For such trivial
       cases, insertion sort is a good choice. Also a good choice for
       future implementations that may wish to indirect through a table.
    */
    for (i = 1; (i < ints); i++) {
      index = polyInts[i];
      j = i;
      while ((j > 0) && (polyInts[j - 1] > index)) {
        polyInts[j] = polyInts[j - 1];
        j--;
      }
      polyInts[j] = index;
    }
    for (i = 0; (i < (ints)); i += 2) {
#if 0
      int minx = polyInts[i];
      int maxx = polyInts[i + 1];
#endif
      /* 2.0.29: back to gdImageLine to prevent segfaults when
         performing a pattern fill */
      proc(polyInts[i], y, polyInts[i + 1], data);
    }
  }
}

} // namespace doc
