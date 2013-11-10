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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "raster/dirty.h"

#include "raster/image.h"
#include "raster/primitives.h"
#include "raster/primitives_fast.h"

#include <algorithm>

namespace raster {

Dirty::Dirty(PixelFormat format, int x1, int y1, int x2, int y2)
  : m_format(format)
  , m_x1(x1), m_y1(y1)
  , m_x2(x2), m_y2(y2)
{
}

Dirty::Dirty(const Dirty& src)
  : m_format(src.m_format)
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

template<typename ImageTraits>
inline bool shrink_row(const Image* image, const Image* image_diff, int& x1, int y, int& x2)
{
  for (; x1<=x2; ++x1) {
    if (get_pixel_fast<ImageTraits>(image, x1, y) !=
        get_pixel_fast<ImageTraits>(image_diff, x1, y))
      break;
  }

  if (x1 > x2)
    return false;

  for (; x2>x1; x2--) {
    if (get_pixel_fast<ImageTraits>(image, x2, y) !=
        get_pixel_fast<ImageTraits>(image_diff, x2, y))
      break;
  }

  return true;
}

Dirty::Dirty(Image* image, Image* image_diff, const gfx::Rect& bounds)
  : m_format(image->getPixelFormat())
  , m_x1(bounds.x), m_y1(bounds.y)
  , m_x2(bounds.x2()-1), m_y2(bounds.y2()-1)
{
  int y, x1, x2;

  for (y=m_y1; y<=m_y2; y++) {
    x1 = m_x1;
    x2 = m_x2;

    bool res;
    switch (image->getPixelFormat()) {
      case IMAGE_RGB:
        res = shrink_row<RgbTraits>(image, image_diff, x1, y, x2);
        break;

      case IMAGE_GRAYSCALE:
        res = shrink_row<GrayscaleTraits>(image, image_diff, x1, y, x2);
        break;

      case IMAGE_INDEXED:
        res = shrink_row<IndexedTraits>(image, image_diff, x1, y, x2);
        break;

      default:
        ASSERT(false && "Not implemented for bitmaps");
        return;
    }
    if (!res)
      continue;

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
  int size = 4+1+2*4+2;         // DWORD+BYTE+WORD[4]+WORD

  RowsList::const_iterator row_it = m_rows.begin();
  RowsList::const_iterator row_end = m_rows.end();
  for (; row_it != row_end; ++row_it) {
    size += 4;                  // y, cols (WORD[2])

    ColsList::const_iterator col_it = (*row_it)->cols.begin();
    ColsList::const_iterator col_end = (*row_it)->cols.end();
    for (; col_it != col_end; ++col_it) {
      size += 4;                // x, w (WORD[2])
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

      uint8_t* address = (uint8_t*)image->getPixelAddress(col->x, row->y);
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

      uint8_t* address = (uint8_t*)image->getPixelAddress(col->x, row->y);
      std::swap_ranges(address, address+getLineSize(col->w), col->data.begin());
    }
  }
}

} // namespace raster
