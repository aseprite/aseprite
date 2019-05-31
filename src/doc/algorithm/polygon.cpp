// Aseprite Document Library
// Copyright (c) 2019 Igara Studio S.A.
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/algo.h"
#include "doc/algorithm/polygon.h"

#include "gfx/point.h"

#include <algorithm>
#include <vector>

namespace doc {

static void addPointsWithoutDuplicatingLastOne(int x, int y, std::vector<gfx::Point>* pts)
{
  const gfx::Point newPoint(x, y);
  if (pts->empty() ||
      *(pts->end()-1) != newPoint) {
    pts->push_back(newPoint);
  }
}

// createUnion() joins a single scan point "x" (a pixel in other words)
// to the input vector "pairs".
// Each pair of elements from "pairs" vector is a representation
// of a scan segment.
// An added scan point "x" to the "pairs" vector is represented
// by two consecutive values of "x".
// Note: "pairs" must be sorted prior execution of this function.
static void createUnion(std::vector<int>& pairs,
                        const int x,
                        int& ints)
{
  bool unionDone = false;
  for (int i=0; i < ints - (ints%2); i+=2) {
    // Case:
    //       pairs[i]      pairs[i+1]
    //           O --------- O
    //   -x-
    if (x < pairs[i]) {
      pairs.insert(pairs.begin()+i, x);
      pairs.insert(pairs.begin()+i, x);
      ints+=2;
      unionDone = true;
      break;
    }
    // Case:
    //           pairs[i]      pairs[i+1]
    //               O --------- O
    //            -x-
    else if (x == pairs[i] - 1) {
      pairs[i] = x;
      unionDone = true;
      break;
    }
    // Case:
    //     pairs[i]      pairs[i+1]
    //        O --------- O
    //                     -x-
    else if (x == pairs[i+1] + 1) {
      unionDone = true;
      if (i + 2 <= ints - 2) {
        if (pairs[i+2] == x) {
          // Simplification:
          pairs.erase(pairs.begin() + (i+1));
          pairs.erase(pairs.begin() + (i+1));
          ints-=2;
          break;
        }
      }
      pairs[i+1] = x;
      break;
    }
    // Case:
    //     pairs[i]      pairs[i+1]
    //        O --------- O
    //             -x-
    else if (x >= pairs[i] && x <= pairs[i+1]) {
      unionDone = true;
      break;
    }
  }
  // Case:
  //    pairs[i]      pairs[i+1]
  //        O --------- O
  //                          -x-
  if (x > pairs[ints-1] && !unionDone) {
    if (pairs.size() == ints) {
      pairs.push_back(x);
      pairs.push_back(x);
    }
    else {
      pairs.insert(pairs.begin() + ints, x);
      pairs.insert(pairs.begin() + ints, x);
    }
    ints+=2;
  }
}

void algorithm::polygon(int vertices, const int* points, void* data, AlgoHLine proc)
{
  if (!vertices)
    return;

  // We load locally the points to "verts" and remove the duplicate points
  // to manage easily the input vector, by the way, we find
  // "ymin" and "ymax" to use them later like vertical scan limits
  // on the scan line loop.
  int ymin = *(points+1);
  int ymax = *(points+1);
  std::vector<gfx::Point> verts(1);
  verts[0].x = *points;
  verts[0].y = *(points+1);
  int verticesAux = vertices;
  for (int i=2; i < vertices*2; i+=2) {
    int last = verts.size() - 1;
    if (verts[last].x == *(points+i) && verts[last].y == *(points + (i+1))) {
      verticesAux--;
      continue;
    }
    verts.push_back(gfx::Point(*(points + i), *(points + (i+1))));
    ASSERT(last + 1 == verts.size() - 1);
    if (verts[last+1].y < ymin)
      ymin = verts[last+1].y;
    if (verts[last+1].y > ymax)
      ymax = verts[last+1].y;
  }
  vertices = verticesAux;

  std::vector<gfx::Point> pts;
  for (int c=0; c < verts.size(); ++c) {
    if (c == verts.size() - 1) {
      algo_line_continuous(verts[verts.size()-1].x,
                           verts[verts.size()-1].y,
                           verts[0].x,
                           verts[0].y,
                           (void*)&pts,
                           (AlgoPixel)&addPointsWithoutDuplicatingLastOne);
      pts.pop_back();
    }
    else {
      algo_line_continuous(verts[c].x,
                           verts[c].y,
                           verts[c+1].x,
                           verts[c+1].y,
                           (void*)&pts,
                           (AlgoPixel)&addPointsWithoutDuplicatingLastOne);
    }
  }

  // Scan Line Loop:
  int y;
  int x1, y1;
  int x2, y2;
  int ind1, ind2;
  int ints;
  std::vector<int> polyInts(pts.size());
  for (y = ymin; y <= ymax; y++) {
    ints = 0;
    for (int i=0; i < pts.size(); i++) {
      if (!i) {
        ind1 = pts.size() - 1;
        ind2 = 0;
      }
      else {
        ind1 = i - 1;
        ind2 = i;
      }
      y1 = pts[ind1].y;
      y2 = pts[ind2].y;
      if (y1 < y2) {
        x1 = pts[ind1].x;
        x2 = pts[ind2].x;
      }
      else if (y1 > y2) {
        y2 = pts[ind1].y;
        y1 = pts[ind2].y;
        x2 = pts[ind1].x;
        x1 = pts[ind2].x;
      }
      else
        continue;

      if ((y >= y1 && y < y2) ||
          (y == ymax && y > y1 && y <= y2)) {
        polyInts[ints] = (int) ((float)((y - y1)*(x2 - x1)) / (float)(y2 - y1) + 0.5f + (float)x1);
        ints++;
      }
    }

    std::sort(polyInts.begin(), polyInts.begin() + ints);

    for (int i=0; i < pts.size(); i++) {
      if (pts[i].y == y)
        createUnion(polyInts, pts[i].x, ints);
    }

    for (int i=0; i < ints; i+=2)
      proc(polyInts[i], y, polyInts[i+1], data);
  }
}

} // namespace doc
