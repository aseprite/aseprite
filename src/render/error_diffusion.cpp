// Aseprite Render Library
// Copyright (c) 2019-2022  Igara Studio S.A
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
#include <vector>

namespace render {

// Predefined error diffusion algorithms
class ErrorDiffusionMatrices {
public:
  static const ErrorDiffusionMatrix& getFloydSteinberg()
  {
    static const ErrorDiffusionMatrix matrix(3,
                                             2,
                                             1,
                                             0,
                                             {
                                               { 0, 0, 7 },
                                               { 3, 5, 1 }
    },
                                             16);
    return matrix;
  }

  static const ErrorDiffusionMatrix& getJarvisJudiceNinke()
  {
    static const ErrorDiffusionMatrix matrix(5,
                                             3,
                                             2,
                                             0,
                                             {
                                               { 0, 0, 0, 7, 5 },
                                               { 3, 5, 7, 5, 3 },
                                               { 1, 3, 5, 3, 1 }
    },
                                             48);
    return matrix;
  }

  static const ErrorDiffusionMatrix& getStucki()
  {
    static const ErrorDiffusionMatrix matrix(5,
                                             3,
                                             2,
                                             0,
                                             {
                                               { 0, 0, 0, 8, 4 },
                                               { 2, 4, 8, 4, 2 },
                                               { 1, 2, 4, 2, 1 }
    },
                                             42);
    return matrix;
  }

  static const ErrorDiffusionMatrix& getAtkinson()
  {
    static const ErrorDiffusionMatrix matrix(4,
                                             3,
                                             1,
                                             0,
                                             {
                                               { 0, 0, 1, 1 },
                                               { 1, 1, 1, 0 },
                                               { 0, 1, 0, 0 }
    },
                                             8);
    return matrix;
  }

  static const ErrorDiffusionMatrix& getBurkes()
  {
    static const ErrorDiffusionMatrix matrix(5,
                                             2,
                                             2,
                                             0,
                                             {
                                               { 0, 0, 0, 8, 4 },
                                               { 2, 4, 8, 4, 2 }
    },
                                             32);
    return matrix;
  }

  static const ErrorDiffusionMatrix& getSierra()
  {
    static const ErrorDiffusionMatrix matrix(5,
                                             3,
                                             2,
                                             0,
                                             {
                                               { 0, 0, 0, 5, 3 },
                                               { 2, 4, 5, 4, 2 },
                                               { 0, 2, 3, 2, 0 }
    },
                                             32);
    return matrix;
  }
};

ErrorDiffusionDither::ErrorDiffusionDither(ErrorDiffusionType type, int transparentIndex)
  : m_transparentIndex(transparentIndex)
  , m_diffusionType(type)
{
}

const ErrorDiffusionMatrix& ErrorDiffusionDither::getCurrentMatrix() const
{
  switch (m_diffusionType) {
    case ErrorDiffusionType::JarvisJudiceNinke:
      return ErrorDiffusionMatrices::getJarvisJudiceNinke();
    case ErrorDiffusionType::Stucki:         return ErrorDiffusionMatrices::getStucki();
    case ErrorDiffusionType::Atkinson:       return ErrorDiffusionMatrices::getAtkinson();
    case ErrorDiffusionType::Burkes:         return ErrorDiffusionMatrices::getBurkes();
    case ErrorDiffusionType::Sierra:         return ErrorDiffusionMatrices::getSierra();
    case ErrorDiffusionType::FloydSteinberg: return ErrorDiffusionMatrices::getFloydSteinberg();
  }
}

void ErrorDiffusionDither::start(const doc::Image* srcImage,
                                 doc::Image* dstImage,
                                 const double factor)
{
  m_srcImage = srcImage;
  m_width = 2 + srcImage->width();

  // Get the current matrix to determine buffer size needed
  const ErrorDiffusionMatrix& matrix = getCurrentMatrix();
  const int bufferRows = matrix.height;

  // Resize error buffers to accommodate the matrix height
  std::vector<int>::size_type bufferSize = m_width * bufferRows;
  for (int i = 0; i < kChannels; ++i)
    m_err[i].resize(bufferSize, 0);

  m_lastY = -1;
  m_currentRowOffset = 0;
  m_factor = int(factor * 100.0);
}

void ErrorDiffusionDither::finish()
{
}

doc::color_t ErrorDiffusionDither::ditherRgbToIndex2D(const int x,
                                                      const int y,
                                                      const doc::RgbMap* rgbmap,
                                                      const doc::Palette* palette,
                                                      const int direction)
{
  const ErrorDiffusionMatrix& matrix = getCurrentMatrix();

  if (y != m_lastY) {
    // Instead of shifting all rows, just advance the circular buffer
    // and clear the row that will be reused
    m_currentRowOffset = (m_currentRowOffset + 1) % matrix.height;

    // Clear only the row that will be used as the "last" row
    int clearRowIndex = (m_currentRowOffset + matrix.height - 1) % matrix.height;
    for (int c = 0; c < kChannels; ++c) {
      int* rowToClear = &m_err[c][m_width * clearRowIndex];
      std::fill(rowToClear, rowToClear + m_width, 0);
    }

    m_lastY = y;
  }

  doc::color_t color = doc::get_pixel_fast<doc::RgbTraits>(m_srcImage, x, y);

  // Get RGB values + quantization error
  int v[kChannels] = { doc::rgba_getr(color),
                       doc::rgba_getg(color),
                       doc::rgba_getb(color),
                       doc::rgba_geta(color) };

  // Add accumulated error (16-bit fixed point) and convert to 0..255
  for (int c = 0; c < kChannels; ++c)
    v[c] = std::clamp(((v[c] << 16) + m_err[c][m_width * m_currentRowOffset + x + 1] + 32767) >> 16,
                      0,
                      255);

  const doc::color_t index = (rgbmap ?
                                rgbmap->mapColor(v[0], v[1], v[2], v[3]) :
                                palette->findBestfit(v[0], v[1], v[2], v[3], m_transparentIndex));

  doc::color_t palColor = palette->getEntry(index);
  if (m_transparentIndex == index || doc::rgba_geta(palColor) == 0) {
    // "color" without alpha
    palColor = (color & doc::rgba_rgb_mask);
  }

  const int quantError[kChannels] = { v[0] - doc::rgba_getr(palColor),
                                      v[1] - doc::rgba_getg(palColor),
                                      v[2] - doc::rgba_getb(palColor),
                                      v[3] - doc::rgba_geta(palColor) };

  const int srcWidth = m_srcImage->width();

  // Distribute error using the configurable matrix
  for (int c = 0; c < kChannels; ++c) {
    const int qerr = quantError[c] * m_factor / 100;

    for (int my = 0; my < matrix.height; ++my) {
      // Use circular buffer indexing
      int bufferRow = (m_currentRowOffset + my) % matrix.height;
      int bufferRowIndex = bufferRow * m_width;

      for (int mx = 0; mx < matrix.width; ++mx) {
        const int coeff = direction > 0 ? matrix.coefficients[my][mx] : matrix.coefficients[my][matrix.width - 1 - mx];
        if (coeff == 0)
          continue;

        const int errorPixelX = x + mx - matrix.centerX;
        const int errorPixelY = y + my - matrix.centerY;

        // Check bounds
        if (errorPixelX < 0 || errorPixelX >= srcWidth)
          continue;
        if (errorPixelY < y)
          continue; // Don't go backwards

        // Calculate error as 16-bit fixed point
        const int errorValue = ((qerr * coeff) << 16) / matrix.divisor;
        const int bufferRow = my;
        const int bufferIndex = bufferRow * m_width + errorPixelX + 1;

        m_err[c][bufferRowIndex + errorPixelX + 1] += errorValue;
      }
    }
  }

  return index;
}

} // namespace render
