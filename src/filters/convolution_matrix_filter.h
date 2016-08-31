// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef FILTERS_CONVOLUTION_MATRIX_FILTER_H_INCLUDED
#define FILTERS_CONVOLUTION_MATRIX_FILTER_H_INCLUDED
#pragma once

#include <vector>

#include "base/ints.h"
#include "base/shared_ptr.h"
#include "filters/filter.h"
#include "filters/tiled_mode.h"

namespace filters {

  class ConvolutionMatrix;

  class ConvolutionMatrixFilter : public Filter {
  public:
    ConvolutionMatrixFilter();

    void setMatrix(const base::SharedPtr<ConvolutionMatrix>& matrix);
    void setTiledMode(TiledMode tiledMode);

    base::SharedPtr<ConvolutionMatrix> getMatrix() { return m_matrix; }
    TiledMode getTiledMode() const { return m_tiledMode; }

    // Filter implementation
    const char* getName();
    void applyToRgba(FilterManager* filterMgr);
    void applyToGrayscale(FilterManager* filterMgr);
    void applyToIndexed(FilterManager* filterMgr);

  private:
    base::SharedPtr<ConvolutionMatrix> m_matrix;
    TiledMode m_tiledMode;
    std::vector<uint8_t*> m_lines;
  };

} // namespace filters

#endif
