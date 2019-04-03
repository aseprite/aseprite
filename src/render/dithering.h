// Aseprite Render Library
// Copyright (c) 2019 Igara Studio S.A.
// Copyright (c) 2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef RENDER_DITHERING_H_INCLUDED
#define RENDER_DITHERING_H_INCLUDED
#pragma once

#include "render/dithering_algorithm.h"
#include "render/dithering_matrix.h"

namespace render {

  class Dithering {
  public:
    Dithering(
      DitheringAlgorithm algorithm = DitheringAlgorithm::None,
      const DitheringMatrix& matrix = DitheringMatrix(),
      double factor = 1.0)
      : m_algorithm(algorithm)
      , m_matrix(matrix)
      , m_factor(factor){ }

    DitheringAlgorithm algorithm() const { return m_algorithm; }
    DitheringMatrix matrix() const { return m_matrix; }
    double factor() const { return m_factor; }

    void algorithm(const DitheringAlgorithm algorithm) { m_algorithm = algorithm; }
    void matrix(const DitheringMatrix& matrix) { m_matrix = matrix; }
    void factor(const double factor) { m_factor = factor; }

  private:
    DitheringAlgorithm m_algorithm;
    DitheringMatrix m_matrix;
    double m_factor;
  };

} // namespace render

#endif
