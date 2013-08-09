/* Aseprite
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

#ifndef FILTERS_CONVOLUTION_MATRIX_H_INCLUDED
#define FILTERS_CONVOLUTION_MATRIX_H_INCLUDED

#include "filters/target.h"

#include <vector>
#include <string>

namespace filters {

  // A convolution matrix which is used by ConvolutionMatrixFilter.
  class ConvolutionMatrix {
  public:
    // TODO warning: this number could be dangerous for big filters
    static const int Precision = 256;

    ConvolutionMatrix(int width, int height);

    const char* getName() const { return m_name.c_str(); }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    int getCenterX() const { return m_cx; }
    int getCenterY() const { return m_cy; }
    int getDiv() const { return m_div; }
    int getBias() const { return m_bias; }
    Target getDefaultTarget() const { return m_defaultTarget; }

    void setName(const char* name) { m_name = name; }
    void setWidth(int width) { m_width = width; }
    void setHeight(int height) { m_height = height; }
    void setCenterX(int cx) { m_cx = cx; }
    void setCenterY(int cy) { m_cy = cy; }
    void setDiv(int div) { m_div = div; }
    void setBias(int bias) { m_bias = bias; }
    void setDefaultTarget(Target target) { m_defaultTarget = target; }

    // Returns a reference to a value in the matrix.  It is very
    // important that values of this matrix are sortered in the
    // following way (e.g. m_width=4, m_height=3):
    //
    //    x0 x1  x2  x3
    //    x4 x5  x6  x7
    //    x8 x9 x10 x11
    //
    // The ConvolutionMatrixFilter expects that memory layout.
    //
    int& value(int x, int y) { return m_data[y*m_width+x]; }
    const int& value(int x, int y) const { return m_data[y*m_width+x]; }

  private:
    std::string m_name;          // Name
    int m_width, m_height;       // Size of the matrix
    int m_cx, m_cy;              // Center of the filter
    int m_div;                   // Division factor
    int m_bias;                  // Addition factor (for offset)
    Target m_defaultTarget;      // Targets by default (look at TARGET_RED_CHANNEL, etc. constants)
    std::vector<int> m_data;     // The matrix with the multiplication factors
  };

} // namespace filters

#endif
