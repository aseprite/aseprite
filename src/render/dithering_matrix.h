// Aseprite Render Library
// Copyright (c) 2020 Igara Studio S.A.
// Copyright (c) 2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef RENDER_DITHERING_MATRIX_H_INCLUDED
#define RENDER_DITHERING_MATRIX_H_INCLUDED
#pragma once

#include <algorithm>
#include <vector>

namespace render {

  class DitheringMatrix {
  public:
    DitheringMatrix()
      : m_rows(1), m_cols(1)
      , m_matrix(1, 1)
      , m_maxValue(1) {
    }

    DitheringMatrix(int rows, int cols)
      : m_rows(rows), m_cols(cols)
      , m_matrix(rows*cols, 0)
      , m_maxValue(rows*cols) {
    }

    DitheringMatrix(int rows, int cols, int maxValue)
      : m_rows(rows), m_cols(cols)
      , m_matrix(rows*cols, 0)
      , m_maxValue(maxValue) {
    }

    int rows() const { return m_rows; }
    int cols() const { return m_cols; }

    int maxValue() const { return m_maxValue; }

    int operator()(int i, int j) const {
      return m_matrix[(i%m_rows)*m_cols + (j%m_cols)];
    }

    int& operator()(int i, int j) {
      return m_matrix[(i%m_rows)*m_cols + (j%m_cols)];
    }

  private:
    int m_rows, m_cols;
    std::vector<int> m_matrix;
    int m_maxValue;
  };

  // Creates a Bayer dither matrix.
  class BayerMatrix : public DitheringMatrix {
    static int D2[4];

  public:
    BayerMatrix(int n) : DitheringMatrix(n, n) {
      for (int i=0; i<n; ++i)
        for (int j=0; j<n; ++j)
          operator()(i, j) = Dn(i, j, n);

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

} // namespace render

#endif
