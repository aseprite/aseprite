/* ASEPRITE
 * Copyright (C) 2001-2013  David Capello
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

#ifndef FILTERS_CONVOLUTION_MATRIX_FILTER_H_INCLUDED
#define FILTERS_CONVOLUTION_MATRIX_FILTER_H_INCLUDED

#include <vector>

#include "base/shared_ptr.h"
#include "filters/filter.h"
#include "filters/tiled_mode.h"

namespace filters {

  class ConvolutionMatrix;

  class ConvolutionMatrixFilter : public Filter {
  public:
    ConvolutionMatrixFilter();

    void setMatrix(const SharedPtr<ConvolutionMatrix>& matrix);
    void setTiledMode(TiledMode tiledMode);

    SharedPtr<ConvolutionMatrix> getMatrix() { return m_matrix; }
    TiledMode getTiledMode() const { return m_tiledMode; }

    // Filter implementation
    const char* getName();
    void applyToRgba(FilterManager* filterMgr);
    void applyToGrayscale(FilterManager* filterMgr);
    void applyToIndexed(FilterManager* filterMgr);

  private:
    SharedPtr<ConvolutionMatrix> m_matrix;
    TiledMode m_tiledMode;
    std::vector<uint8_t*> m_lines;
  };

} // namespace filters

#endif
