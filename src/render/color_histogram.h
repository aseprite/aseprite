// Aseprite Render Library
// Copyright (c)      2020 Igara Studio S.A.
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef RENDER_COLOR_HISTOGRAM_H_INCLUDED
#define RENDER_COLOR_HISTOGRAM_H_INCLUDED
#pragma once

#include <limits>
#include <vector>

#include "doc/color.h"
#include "doc/image.h"
#include "doc/image_traits.h"
#include "doc/palette.h"

#include "render/median_cut.h"

namespace render {
using namespace doc;

template<int RBits, // Number of bits for each component in the histogram
         int GBits,
         int BBits,
         int ABits>
class ColorHistogram {
public:
  // Number of elements in histogram for each RGB component
  enum {
    RElements = 1 << RBits,
    GElements = 1 << GBits,
    BElements = 1 << BBits,
    AElements = 1 << ABits
  };

  ColorHistogram()
    : m_histogram(RElements * GElements * BElements * AElements, 0)
    , m_useHighPrecision(true)
  {
  }

  // Returns the number of points in the specified histogram
  // entry. Each rgba-index is in the range of the histogram, e.g.
  // r=[0,RElements), g=[0,GElements), etc.
  std::size_t at(int r, int g, int b, int a) const
  {
    return m_histogram[histogramIndex(r, g, b, a)];
  }

  // Add the specified "color" in the histogram as many times as the
  // specified value in "count".
  void addSamples(doc::color_t color, std::size_t count = 1)
  {
    int i = histogramIndex(color);

    if (m_histogram[i] < std::numeric_limits<std::size_t>::max() - count) // Avoid overflow
      m_histogram[i] += count;
    else
      m_histogram[i] = std::numeric_limits<std::size_t>::max();

    // Accurate colors are used only for less than 256 colors.  If the
    // image has more than 256 colors the m_histogram is used
    // instead.
    if (m_useHighPrecision) {
      std::vector<doc::color_t>::iterator it =
        std::find(m_highPrecision.begin(), m_highPrecision.end(), color);

      // The color is not in the high-precision table
      if (it == m_highPrecision.end()) {
        if (m_highPrecision.size() < 256) {
          m_highPrecision.push_back(color);
        }
        else {
          // In this case we reach the limit for the high-precision histogram.
          m_useHighPrecision = false;
        }
      }
    }
  }

  // Creates a set of entries for the given palette in the given range
  // with the more important colors in the histogram. Returns the
  // number of used entries in the palette (maybe the range [from,to]
  // is more than necessary).
  int createOptimizedPalette(Palette* palette)
  {
    // Can we use the high-precision table?
    if (m_useHighPrecision && int(m_highPrecision.size()) <= palette->size()) {
      for (int i = 0; i < (int)m_highPrecision.size(); ++i)
        palette->setEntry(i, m_highPrecision[i]);

      return m_highPrecision.size();
    }
    // OK, we have to use the histogram and some algorithm (like
    // median-cut) to quantize "optimal" colors.
    else {
      std::vector<doc::color_t> result;
      median_cut(*this, palette->size(), result);

      for (int i = 0; i < (int)result.size(); ++i)
        palette->setEntry(i, result[i]);

      return result.size();
    }
  }

  bool isHighPrecision() { return m_useHighPrecision; }
  int highPrecisionSize() { return m_highPrecision.size(); }

private:
  // Converts input color in a index for the histogram. It reduces
  // each 8-bit component to the resolution given in the template
  // parameters.
  std::size_t histogramIndex(doc::color_t color) const
  {
    return histogramIndex((rgba_getr(color) >> (8 - RBits)),
                          (rgba_getg(color) >> (8 - GBits)),
                          (rgba_getb(color) >> (8 - BBits)),
                          (rgba_geta(color) >> (8 - ABits)));
  }

  std::size_t histogramIndex(int r, int g, int b, int a) const
  {
    return r | (g << RBits) | (b << (RBits + GBits)) | (a << (RBits + GBits + BBits));
  }

  // 3D histogram (the index in the histogram is calculated through histogramIndex() function).
  std::vector<std::size_t> m_histogram;

  // High precision histogram to create an accurate palette if RGB
  // source images contains less than 256 colors.
  std::vector<doc::color_t> m_highPrecision;

  // True if we can use m_highPrecision still (it means that the
  // number of different samples is less than 256 colors still).
  bool m_useHighPrecision;
};

} // namespace render

#endif
