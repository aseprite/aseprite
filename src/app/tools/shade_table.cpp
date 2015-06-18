// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/tools/shade_table.h"

#include "app/color_swatches.h"
#include "doc/palette.h"

namespace app {
namespace tools {

ShadeTable8::ShadeTable8(const app::ColorSwatches& colorSwatches, ShadingMode mode)
{
  m_left.resize(256);           // TODO could we have more than 256 indexes?
  m_right.resize(256);

  for (size_t i=0; i<256; ++i) {
    m_left[i] = i;
    m_right[i] = i;
  }

  size_t n = colorSwatches.size();
  for (size_t i=0; i<n; ++i) {
    size_t leftDst;
    size_t rightDst;

    if (i == 0) {
      if (mode == kRotateShadingMode)
        leftDst = n-1;
      else
        leftDst = 0;
    }
    else
      leftDst = i-1;

    if (i == n-1) {
      if (mode == kRotateShadingMode)
        rightDst = 0;
      else
        rightDst = n-1;
    }
    else
      rightDst = i+1;

    size_t src = colorSwatches[i].getIndex();
    m_left[src] = colorSwatches[leftDst].getIndex();
    m_right[src] = colorSwatches[rightDst].getIndex();
  }
}

} // namespace tools
} // namespace app
