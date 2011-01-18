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

#include "config.h"

#include <algorithm>

#include "raster/dirty.h"
#include "raster/image.h"

Dirty::Dirty(int imgtype, int x1, int y1, int x2, int y2)
  : m_imgtype(imgtype)
  , m_x1(x1), m_y1(y1)
  , m_x2(x2), m_y2(y2)
{
}

Dirty::Dirty(const Dirty& src)
  : m_imgtype(src.m_imgtype)
  , m_x1(src.m_x1), m_y1(src.m_y1)
  , m_x2(src.m_x2), m_y2(src.m_y2)
{
  m_rows.resize(src.m_rows.size());

  for (size_t v=0; v<m_rows.size(); ++v) {
    const Row& srcRow = src.getRow(v);
    Row* row = new Row(srcRow.y);

    row->cols.resize(srcRow.cols.size());

    for (size_t u=0; u<row->cols.size(); ++u) {
      Col* col = new Col(*srcRow.cols[u]);
      row->cols[u] = col;
    }

    m_rows[v] = row;
  }
}

Dirty::Dirty(Image* image, Image* image_diff)
  : m_imgtype(image->imgtype)
  , m_x1(0), m_y1(0)
  , m_x2(image->w-1), m_y2(image->h-1)
{
  int x, y, x1, x2;

  for (y=0; y<image->h; y++) {
    x1 = -1;
    for (x=0; x<image->w; x++) {
      if (image_getpixel(image, x, y) != image_getpixel(image_diff, x, y)) {
	x1 = x;
	break;
      }
    }
    if (x1 < 0)
      continue;

    for (x2=image->w-1; x2>x1; x2--) {
      if (image_getpixel(image, x2, y) != image_getpixel(image_diff, x2, y))
	break;
    }

    Col* col = new Col(x1, x2-x1+1);
    col->data.resize(getLineSize(col->w));

    Row* row = new Row(y);
    row->cols.push_back(col);

    m_rows.push_back(row);
  }
}

Dirty::~Dirty()
{
  RowsList::iterator row_it = m_rows.begin();
  RowsList::iterator row_end = m_rows.end();

  for (; row_it != row_end; ++row_it) {
    ColsList::iterator col_it = (*row_it)->cols.begin();
    ColsList::iterator col_end = (*row_it)->cols.end();

    for (; col_it != col_end; ++col_it)
      delete *col_it;

    delete *row_it;
  }
}

int Dirty::getMemSize() const
{
  int size = 4+1+2*4+2;		// DWORD+BYTE+WORD[4]+WORD

  RowsList::const_iterator row_it = m_rows.begin();
  RowsList::const_iterator row_end = m_rows.end();
  for (; row_it != row_end; ++row_it) {
    size += 4;			// y, cols (WORD[2])

    ColsList::const_iterator col_it = (*row_it)->cols.begin();
    ColsList::const_iterator col_end = (*row_it)->cols.end();
    for (; col_it != col_end; ++col_it) {
      size += 4;		// x, w (WORD[2])
      size += getLineSize((*col_it)->w);
    }
  }

  return size;
}

void Dirty::saveImagePixels(Image* image)
{
  RowsList::iterator row_it = m_rows.begin();
  RowsList::iterator row_end = m_rows.end();
  for (; row_it != row_end; ++row_it) {
    Row* row = *row_it;

    ColsList::iterator col_it = (*row_it)->cols.begin();
    ColsList::iterator col_end = (*row_it)->cols.end();
    for (; col_it != col_end; ++col_it) {
      Col* col = *col_it;

      ase_uint8* address = (ase_uint8*)image_address(image, col->x, row->y);
      std::copy(address, address+getLineSize(col->w), col->data.begin());
    }
  }
}

void Dirty::swapImagePixels(Image* image)
{
  RowsList::iterator row_it = m_rows.begin();
  RowsList::iterator row_end = m_rows.end();
  for (; row_it != row_end; ++row_it) {
    Row* row = *row_it;

    ColsList::iterator col_it = (*row_it)->cols.begin();
    ColsList::iterator col_end = (*row_it)->cols.end();
    for (; col_it != col_end; ++col_it) {
      Col* col = *col_it;

      ase_uint8* address = (ase_uint8*)image_address(image, col->x, row->y);
      std::swap_ranges(address, address+getLineSize(col->w), col->data.begin());
    }
  }
}
