/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#ifndef COMMANDS_FILTERS_CONVOLUTION_MATRIX_STOCK_H_INCLUDED
#define COMMANDS_FILTERS_CONVOLUTION_MATRIX_STOCK_H_INCLUDED

#include <vector>

#include "base/shared_ptr.h"

class ConvolutionMatrix;

// A container of all convolution matrices in the convmatr.def file.
class ConvolutionMatrixStock
{
public:
  typedef std::vector<SharedPtr<ConvolutionMatrix> >::iterator iterator;
  typedef std::vector<SharedPtr<ConvolutionMatrix> >::const_iterator const_iterator;

  ConvolutionMatrixStock();
  virtual ~ConvolutionMatrixStock();

  iterator begin() { return m_matrices.begin(); }
  iterator end() { return m_matrices.end(); }
  const_iterator begin() const { return m_matrices.begin(); }
  const_iterator end() const { return m_matrices.end(); }

  SharedPtr<ConvolutionMatrix> getByName(const char* name);

  void reloadStock();
  void cleanStock();

private:
  std::vector<SharedPtr<ConvolutionMatrix> > m_matrices;
};

#endif
