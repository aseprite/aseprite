// Aseprite Render Library
// Copyright (c) 2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "render/ordered_dither.h"

namespace render {

// Base 2x2 dither matrix, called D(2):
int BayerMatrix::D2[4] = { 0, 2,
                           3, 1 };

void dither_rgb_image_to_indexed(
  DitheringAlgorithmBase& algorithm,
  const DitheringMatrix& matrix,
  const doc::Image* srcImage,
  doc::Image* dstImage,
  int u, int v,
  const doc::RgbMap* rgbmap,
  const doc::Palette* palette,
  TaskDelegate* delegate)
{
  const doc::LockImageBits<doc::RgbTraits> srcBits(srcImage);
  doc::LockImageBits<doc::IndexedTraits> dstBits(dstImage);
  auto srcIt = srcBits.begin();
  auto dstIt = dstBits.begin();
  int w = srcImage->width();
  int h = srcImage->height();

  for (int y=0; y<h; ++y) {
    for (int x=0; x<w; ++x, ++srcIt, ++dstIt) {
      ASSERT(srcIt != srcBits.end());
      ASSERT(dstIt != dstBits.end());
      *dstIt = algorithm.ditherRgbPixelToIndex(matrix, *srcIt, x+u, y+v, rgbmap, palette);

      if (delegate) {
        if (!delegate->continueTask())
          return;
      }
    }

    if (delegate) {
      delegate->notifyTaskProgress(
        double(y+1) / double(h));
    }
  }
}

} // namespace render
