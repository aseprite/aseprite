/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef RASTER_COLOR_HISTOGRAM_H_INCLUDED
#define RASTER_COLOR_HISTOGRAM_H_INCLUDED

#include <vector>
#include "raster/image.h"
#include "raster/image_traits.h"
#include "raster/median_cut.h"
#include "raster/palette.h"

namespace quantization {

template<int RBits, int GBits, int BBits> // Number of bits for each component in the histogram
class ColorHistogram
{
public:
  // Number of elements in histogram for each RGB component
  enum {
    RElements = 1 << RBits,
    GElements = 1 << GBits,
    BElements = 1 << BBits
  };

  ColorHistogram()
    : m_histogram(RElements*GElements*BElements, 0)
    , m_useHighPrecision(true)
  {
  }

  // Returns the number of points in the specified histogram
  // entry. Each index (i, j, k) is in the range of the
  // histogram i=[0,RElements), etc.
  size_t at(int i, int j, int k) const
  {
    return m_histogram[histogramIndex(i, j, k)];
  }

  // Add the specified "color" in the histogram as many times as the
  // specified value in "count".
  void addSamples(uint32_t color, size_t count = 1)
  {
    int i = histogramIndex(color);

    if (m_histogram[i] < std::numeric_limits<size_t>::max()-count) // Avoid overflow
      m_histogram[i] += count;
    else
      m_histogram[i] = std::numeric_limits<size_t>::max();

    // Accurate colors are used only for less than 256 colors.  If the
    // image has more than 256 colors the m_histogram is used
    // instead.
    if (m_useHighPrecision) {
      std::vector<uint32_t>::iterator it =
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
  int createOptimizedPalette(Palette* palette, int from, int to)
  {
    // Can we use the high-precision table?
    if (m_useHighPrecision && int(m_highPrecision.size()) <= (to-from+1)) {
      for (int i=0; i<(int)m_highPrecision.size(); ++i)
        palette->setEntry(from+i, m_highPrecision[i]);

      return m_highPrecision.size();
    }
    // OK, we have to use the histogram and some algorithm (like
    // median-cut) to quantize "optimal" colors.
    else {
      std::vector<uint32_t> result;
      median_cut(*this, to-from+1, result);

      for (int i=0; i<(int)result.size(); ++i)
        palette->setEntry(from+i, result[i]);

      return result.size();
    }
  }

private:
  // Converts input color in a index for the histogram. It reduces
  // each 8-bit component to the resolution given in the template
  // parameters.
  size_t histogramIndex(uint32_t color) const
  {
    return histogramIndex((_rgba_getr(color) >> (8 - RBits)),
                          (_rgba_getg(color) >> (8 - GBits)),
                          (_rgba_getb(color) >> (8 - BBits)));
  }

  size_t histogramIndex(int i, int j, int k) const
  {
    return i | (j << RBits) | (k << (RBits+GBits));
  }

  // 3D histogram (the index in the histogram is calculated through histogramIndex() function).
  std::vector<size_t> m_histogram;

  // High precision histogram to create an accurate palette if RGB
  // source images contains less than 256 colors.
  std::vector<uint32_t> m_highPrecision;

  // True if we can use m_highPrecision still (it means that the
  // number of different samples is less than 256 colors still).
  bool m_useHighPrecision;
};

}

#endif
