// Aseprite Render Library
// Copyright (c) 2001-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef RENDER_ORDERED_DITHER_H_INCLUDED
#define RENDER_ORDERED_DITHER_H_INCLUDED
#pragma once

#include "doc/color.h"
#include "doc/image_impl.h"
#include "doc/palette.h"
#include "doc/rgbmap.h"

namespace render {

  // Creates a Bayer dither matrix.
  template<int N>
  class BayerMatrix {
    static int D2[4];

    int m_matrix[N*N];

  public:
    int maxValue() const { return N*N; }

    BayerMatrix() {
      int c = 0;
      for (int i=0; i<N; ++i)
        for (int j=0; j<N; ++j)
          m_matrix[c++] = Dn(i, j, N);
    }

    int operator()(int i, int j) const {
      return m_matrix[(i%N)*N + (j%N)];
    }

    int operator[](int i) const {
      return m_matrix[i];
    }

  private:
    int Dn(int i, int j, int n) const {
      ASSERT(i >= 0 && i < n);
      ASSERT(j >= 0 && j < n);

      if (n == 2)
        return D2[i*2 + j];
      else
        return
          + 4*Dn(i%(n/2), j%(n/2), n/2)
          +   Dn(i/(n/2), j/(n/2), 2);
    }
  };

  // Base 2x2 dither matrix, called D(2):
  template<int N>
  int BayerMatrix<N>::D2[4] = { 0, 2,
                                3, 1 };

  class OrderedDither {
    static int colorDistance(int r1, int g1, int b1, int a1,
                             int r2, int g2, int b2, int a2) {
      // The factor for RGB components came from doc::rba_luma()
      return int((r1-r2) * (r1-r2) * 21 + // 2126
                 (g1-g2) * (g1-g2) * 71 + // 7152
                 (b1-b2) * (b1-b2) *  7 + //  722
                 (a1-a2) * (a1-a2));
    }

  public:
    OrderedDither(int transparentIndex = -1) : m_transparentIndex(transparentIndex) {
    }

    template<typename Matrix>
    doc::color_t ditherRgbPixelToIndex(
      const Matrix& matrix,
      doc::color_t color,
      int x, int y,
      const doc::RgbMap* rgbmap,
      const doc::Palette* palette) {
      // Alpha=0, output transparent color
      if (m_transparentIndex >= 0 && !doc::rgba_geta(color))
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
      int threshold = matrix(x, y);

      return (d > threshold ? nearest2idx:
                              nearest1idx);
    }

    template<typename Matrix>
    void ditherRgbImageToIndexed(const Matrix& matrix,
                                 const doc::Image* srcImage,
                                 doc::Image* dstImage,
                                 int u, int v,
                                 const doc::RgbMap* rgbmap,
                                 const doc::Palette* palette,
                                 bool* stopFlag = nullptr) {
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
          *dstIt = ditherRgbPixelToIndex(matrix, *srcIt, x+u, y+v, rgbmap, palette);
        }
        if (stopFlag && *stopFlag)
          break;
      }
    }

  private:
    int m_transparentIndex;
  };

} // namespace render

#endif
