// Aseprite Render Library
// Copyright (c) 2019-2020  Igara Studio S.A
// Copyright (c) 2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "render/error_diffusion.h"

#include "gfx/hsl.h"
#include "gfx/rgb.h"

#include <algorithm>

namespace render {

ErrorDiffusionDither::ErrorDiffusionDither(int transparentIndex)
  : m_transparentIndex(transparentIndex)
{
}

void ErrorDiffusionDither::start(
  const doc::Image* srcImage,
  doc::Image* dstImage,
  const double factor)
{
  m_srcImage = srcImage;
  m_width = 2+srcImage->width();
  for (int i=0; i<kChannels; ++i)
    m_err[i].resize(m_width*2, 0);
  m_lastY = -1;
  m_factor = int(factor * 100.0);
}

void ErrorDiffusionDither::finish()
{
}

doc::color_t ErrorDiffusionDither::ditherRgbToIndex2D(
  const int x, const int y,
  const doc::RgbMap* rgbmap,
  const doc::Palette* palette)
{
  if (y != m_lastY) {
    for (int i=0; i<kChannels; ++i) {
      int* row0 = &m_err[i][0];
      int* row1 = row0 + m_width;
      int* end1 = row1 + m_width;
      std::copy(row1, end1, row0);
      std::fill(row1, end1, 0);
    }
    m_lastY = y;
  }

  doc::color_t color =
    doc::get_pixel_fast<doc::RgbTraits>(m_srcImage, x, y);

  // Get RGB values + quatization error
  int v[kChannels] = {
    doc::rgba_getr(color),
    doc::rgba_getg(color),
    doc::rgba_getb(color),
    doc::rgba_geta(color)
  };
  for (int i=0; i<kChannels; ++i) {
    v[i] += m_err[i][x+1];
    v[i] = base::clamp(v[i], 0, 255);
  }

  const doc::color_t index =
    (rgbmap ? rgbmap->mapColor(v[0], v[1], v[2], v[3]):
              palette->findBestfit(v[0], v[1], v[2], v[3], m_transparentIndex));

  doc::color_t palColor = palette->getEntry(index);
  if (m_transparentIndex == index || doc::rgba_geta(palColor) == 0) {
    // "color" without alpha
    palColor = (color & doc::rgba_rgb_mask);
  }

  const int quantError[kChannels] = {
    v[0] - doc::rgba_getr(palColor),
    v[1] - doc::rgba_getg(palColor),
    v[2] - doc::rgba_getb(palColor),
    v[3] - doc::rgba_geta(palColor)
  };

  // TODO using Floyd-Steinberg matrix here but it should be configurable
  for (int i=0; i<kChannels; ++i) {
    int* err = &m_err[i][x];
    const int q = quantError[i] * m_factor / 100;
    const int a = q * 7 / 16;
    const int b = q * 3 / 16;
    const int c = q * 5 / 16;
    const int d = q * 1 / 16;

    if (y & 1) {
      err[0        ] += a;
      err[m_width+2] += b;
      err[m_width+1] += c;
      err[m_width  ] += d;
    }
    else {
      err[       +2] += a;
      err[m_width  ] += b;
      err[m_width+1] += c;
      err[m_width+2] += d;
    }
  }

  return index;
}

} // namespace render
