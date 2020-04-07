// Aseprite Render Library
// Copyright (c) 2020  Igara Studio S.A.
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/clamp.h"
#include "base/debug.h"
#include "render/zoom.h"

namespace render {

static int scales[][2] = {
  { 1, 64 },
  { 1, 48 },
  { 1, 32 },
  { 1, 24 },
  { 1, 16 },
  { 1, 12 },
  { 1, 8 },
  { 1, 6 },
  { 1, 5 },
  { 1, 4 },
  { 1, 3 },
  { 1, 2 },
  { 1, 1 }, // 100%
  { 2, 1 },
  { 3, 1 },
  { 4, 1 },
  { 5, 1 },
  { 6, 1 },
  { 8, 1 },
  { 12, 1 },
  { 16, 1 },
  { 24, 1 },
  { 32, 1 },
  { 48, 1 },
  { 64, 1 },
};

static int scales_size = sizeof(scales) / sizeof(scales[0]);

Zoom::Zoom(int num, int den)
  : m_num(num)
  , m_den(den)
{
  ASSERT(m_num > 0);
  ASSERT(m_den > 0);
  m_internalScale = scale();
}

bool Zoom::in()
{
  int i = linearScale();
  if (i < scales_size-1) {
    ++i;
    m_num = scales[i][0];
    m_den = scales[i][1];
    m_internalScale = scale();
    return true;
  }
  else
    return false;
}

bool Zoom::out()
{
  int i = linearScale();
  if (i > 0) {
    --i;
    m_num = scales[i][0];
    m_den = scales[i][1];
    m_internalScale = scale();
    return true;
  }
  else
    return false;
}

int Zoom::linearScale() const
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
  Zoom zoom = fromLinearScale(findClosestLinearScale(scale));
  zoom.m_internalScale = scale;
  return zoom;
}

// static
Zoom Zoom::fromLinearScale(int i)
{
  i = base::clamp(i, 0, scales_size-1);
  return Zoom(scales[i][0], scales[i][1]);
}

// static
int Zoom::findClosestLinearScale(double scale)
{
  for (int i=1; i<scales_size-1; ++i) {
    double min = double(scales[i-1][0]) / double(scales[i-1][1]);
    double mid = double(scales[i  ][0]) / double(scales[i  ][1]);
    double max = double(scales[i+1][0]) / double(scales[i+1][1]);

    if (scale >= (min+mid)/2.0 &&
        scale <= (mid+max)/2.0)
      return i;
  }
  if (scale < 1.0)
    return 0;
  else
    return scales_size-1;
}

int Zoom::linearValues()
{
  return scales_size;
}

} // namespace render
