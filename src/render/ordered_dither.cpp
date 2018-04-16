// Aseprite Render Library
// Copyright (c) 2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "render/ordered_dither.h"

#include "base/base.h"

#include <algorithm>
#include <limits>
#include <vector>

namespace render {

// Base 2x2 dither matrix, called D(2):
int BayerMatrix::D2[4] = { 0, 2,
                           3, 1 };

static int colorDistance(int r1, int g1, int b1, int a1,
                         int r2, int g2, int b2, int a2)
{
  int result = 0;

  // The factor for RGB components came from doc::rba_luma()
  if (a1 && a2) {
    result += int(std::abs(r1-r2) * 2126 +
                  std::abs(g1-g2) * 7152 +
                  std::abs(b1-b2) *  722);
  }

  result += (std::abs(a1-a2) * 20000);
  return result;
}

OrderedDither::OrderedDither(int transparentIndex)
  : m_transparentIndex(transparentIndex)
{
}

doc::color_t OrderedDither::ditherRgbPixelToIndex(
  const DitheringMatrix& matrix,
  const doc::color_t color,
  const int x,
  const int y,
  const doc::RgbMap* rgbmap,
  const doc::Palette* palette)
{
  // Alpha=0, output transparent color
  if (m_transparentIndex >= 0 &&
      doc::rgba_geta(color) == 0)
    return m_transparentIndex;

  // Get the nearest color in the palette with the given RGB
  // values.
  int r = doc::rgba_getr(color);
  int g = doc::rgba_getg(color);
  int b = doc::rgba_getb(color);
  int a = doc::rgba_geta(color);
  doc::color_t nearest1idx =
    (rgbmap ? rgbmap->mapColor(r, g, b, a):
              palette->findBestfit(r, g, b, a, m_transparentIndex));

  doc::color_t nearest1rgb = palette->getEntry(nearest1idx);
  int r1 = doc::rgba_getr(nearest1rgb);
  int g1 = doc::rgba_getg(nearest1rgb);
  int b1 = doc::rgba_getb(nearest1rgb);
  int a1 = doc::rgba_geta(nearest1rgb);

  // Between the original color ('color' parameter) and 'nearest'
  // index, we have an error (r1-r, g1-g, b1-b). Here we try to
  // find the other nearest color with the same error but with
  // different sign.
  int r2 = r - (r1-r);
  int g2 = g - (g1-g);
  int b2 = b - (b1-b);
  int a2 = a - (a1-a);
  r2 = MID(0, r2, 255);
  g2 = MID(0, g2, 255);
  b2 = MID(0, b2, 255);
  a2 = MID(0, a2, 255);
  doc::color_t nearest2idx =
    (rgbmap ? rgbmap->mapColor(r2, g2, b2, a2):
              palette->findBestfit(r2, g2, b2, a2, m_transparentIndex));

  // If both possible RGB colors use the same index, we cannot
  // make any dither with these two colors.
  if (nearest1idx == nearest2idx)
    return nearest1idx;

  doc::color_t nearest2rgb = palette->getEntry(nearest2idx);
  r2 = doc::rgba_getr(nearest2rgb);
  g2 = doc::rgba_getg(nearest2rgb);
  b2 = doc::rgba_getb(nearest2rgb);
  a2 = doc::rgba_geta(nearest2rgb);

  // Here we calculate the distance between the original 'color'
  // and 'nearest1rgb'. The maximum possible distance is given by
  // the distance between 'nearest1rgb' and 'nearest2rgb'.
  int d = colorDistance(r1, g1, b1, a1, r, g, b, a);
  int D = colorDistance(r1, g1, b1, a1, r2, g2, b2, a2);
  if (D == 0)
    return nearest1idx;

  // We convert the d/D factor to the matrix range to compare it
  // with the threshold. If d > threshold, it means that we're
  // closer to 'nearest2rgb' than to 'nearest1rgb'.
  d = matrix.maxValue() * d / D;
  int threshold = matrix(y, x);

  return (d > threshold ? nearest2idx:
                          nearest1idx);
}

OrderedDither2::OrderedDither2(int transparentIndex)
  : m_transparentIndex(transparentIndex)
{
}

// New ordered dithering algorithm using the best match between two
// indexes to create a mix that can reproduce the original RGB
// color.
//
// TODO it's too slow for big color palettes:
//      O(W*H*P) where P is the number of palette entries
//
// Some ideas from:
// http://bisqwit.iki.fi/story/howto/dither/jy/
//
doc::color_t OrderedDither2::ditherRgbPixelToIndex(
  const DitheringMatrix& matrix,
  const doc::color_t color,
  const int x,
  const int y,
  const doc::RgbMap* rgbmap,
  const doc::Palette* palette)
{
  // Alpha=0, output transparent color
  if (m_transparentIndex >= 0 &&
      doc::rgba_geta(color) == 0) {
    return m_transparentIndex;
  }

  // Get RGBA values
  const int r = doc::rgba_getr(color);
  const int g = doc::rgba_getg(color);
  const int b = doc::rgba_getb(color);
  const int a = doc::rgba_geta(color);

  // Find the best palette entry for the given color.
  const int index =
    (rgbmap ? rgbmap->mapColor(r, g, b, a):
              palette->findBestfit(r, g, b, a, m_transparentIndex));

  const doc::color_t color0 = palette->getEntry(index);
  const int r0 = doc::rgba_getr(color0);
  const int g0 = doc::rgba_getg(color0);
  const int b0 = doc::rgba_getb(color0);
  const int a0 = doc::rgba_geta(color0);

  // Find the best combination between the found nearest index and
  // an alternative palette color to create the original RGB color.
  int bestMix = 0;
  int altIndex = -1;
  int closestDistance = std::numeric_limits<int>::max();
  for (int i=0; i<palette->size(); ++i) {
    if (i == m_transparentIndex)
      continue;

    const doc::color_t color1 = palette->getEntry(i);
    const int r1 = doc::rgba_getr(color1);
    const int g1 = doc::rgba_getg(color1);
    const int b1 = doc::rgba_getb(color1);
    const int a1 = doc::rgba_geta(color1);

    // Find the best "mix factor" between both palette indexes to
    // reproduce the original RGB color. A possible algorithm
    // would be to iterate all possible mix factors from 0 to
    // maxMixValue, but this is too slow, so we try to figure out
    // a good mix factor using the RGB values of color0 and
    // color1.
    int maxMixValue = matrix.maxValue();

    int mix = 0;
    int div = 0;
    // If Alpha=0, RGB values are not representative for this entry.
    if (a && a0 && a1) {
      if (r1-r0) mix += 2126 * maxMixValue * (r-r0) / (r1-r0), div += 2126;
      if (g1-g0) mix += 7152 * maxMixValue * (g-g0) / (g1-g0), div += 7152;
      if (b1-b0) mix +=  722 * maxMixValue * (b-b0) / (b1-b0), div +=  722;
    }
    if (a1-a0) mix += 20000 * maxMixValue * (a-a0) / (a1-a0), div += 20000;
    if (mix) {
      if (div)
        mix /= div;
      mix = MID(0, mix, maxMixValue);
    }

    const int rM = r0 + (r1-r0) * mix / maxMixValue;
    const int gM = g0 + (g1-g0) * mix / maxMixValue;
    const int bM = b0 + (b1-b0) * mix / maxMixValue;
    const int aM = a0 + (a1-a0) * mix / maxMixValue;
    const int d =
      colorDistance(r, g, b, a, rM, gM, bM, aM)
      // Don't use an alternative index if it's too far away from the first index
      + colorDistance(r0, g0, b0, a0, r1, g1, b1, a1) / 10;

    if (closestDistance > d) {
      closestDistance = d;
      bestMix = mix;
      altIndex = i;
    }
  }

  // Using the bestMix factor the dithering matrix tells us if we
  // should paint with altIndex or index in this x,y position.
  if (altIndex >= 0 && matrix(y, x) < bestMix)
    return altIndex;
  else
    return index;
}

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
