// Aseprite Render Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "render/zoom.h"

namespace render {

static int scales[][2] = {
  { 1, 64 },
  { 1, 32 },
  { 1, 16 },
  { 1, 8 },
  { 1, 6 },
  { 1, 4 },
  { 1, 3 },
  { 1, 2 },
  { 1, 1 }, // 100%
  { 2, 1 },
  { 3, 1 },
  { 4, 1 },
  { 6, 1 },
  { 8, 1 },
  { 16, 1 },
  { 32, 1 },
  { 64, 1 },
};

static int scales_size = sizeof(scales) / sizeof(scales[0]);

void Zoom::in()
{
  int i = linearScale();
  if (i < scales_size-1) {
    ++i;
    m_num = scales[i][0];
    m_den = scales[i][1];
  }
}

void Zoom::out()
{
  int i = linearScale();
  if (i > 0) {
    --i;
    m_num = scales[i][0];
    m_den = scales[i][1];
  }
}

int Zoom::linearScale()
{
  for (int i=0; i<scales_size; ++i) {
    // Exact match
    if (scales[i][0] == m_num &&
        scales[i][1] == m_den) {
      return i;
    }
  }
  return findClosestLinearScale(scale());
}

// static
Zoom Zoom::fromScale(double scale)
{
  return fromLinearScale(findClosestLinearScale(scale));
}

// static
Zoom Zoom::fromLinearScale(int i)
{
  i = MID(0, i, scales_size-1);
  return Zoom(scales[i][0], scales[i][1]);
}

// static
int Zoom::findClosestLinearScale(double scale)
{
  for (int i=0; i<scales_size-1; ++i) {
    double min = double(scales[i  ][0]) / double(scales[i  ][1]) - 0.5;
    double max = double(scales[i+1][0]) / double(scales[i+1][1]) - 0.5;
    if (scale >= min && scale <= max)
      return i;
  }
  if (scale < 1.0)
    return 0;
  else
    return scales_size-1;
}

} // namespace render
