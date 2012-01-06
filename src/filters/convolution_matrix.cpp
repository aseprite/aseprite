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

#include "config.h"

#include "filters/convolution_matrix.h"

ConvolutionMatrix::ConvolutionMatrix(int width, int height)
  : m_data(width*height, 0)
  , m_width(width)
  , m_height(height)
  , m_cx(width/2)
  , m_cy(height/2)
  , m_div(ConvolutionMatrix::Precision)
  , m_bias(0)
  , m_defaultTarget(0)
{
}
