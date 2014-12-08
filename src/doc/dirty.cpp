// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/dirty.h"

#include "doc/image.h"
#include "doc/primitives.h"
#include "doc/primitives_fast.h"

#include <algorithm>

namespace doc {

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

Dirty::Dirty(PixelFormat format, const gfx::Rect& bounds)
  : m_format(format)
  , m_bounds(bounds)
{
}

Dirty::Dirty(const Dirty& src)
  : m_format(src.m_format)
  , m_bounds(src.m_bounds)
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

Dirty::Dirty(Image* image1, Image* image2, const gfx::Rect& bounds)
  : m_format(image1->pixelFormat())
  , m_bounds(bounds)
{
  initialize(image1, image2, gfx::Region(bounds));
}

Dirty::Dirty(Image* image1, Image* image2, const gfx::Region& region)
  : m_format(image1->pixelFormat())
  , m_bounds(region.bounds())
{
  initialize(image1, image2, region);
}

void Dirty::initialize(Image* image1, Image* image2, const gfx::Region& region)
{
  int y, x1, x2;

  for (const auto& rc : region) {
    for (y=rc.y; y<rc.y+rc.h; ++y) {
      x1 = rc.x;
      x2 = rc.x+rc.w-1;

      bool res;
      switch (m_format) {
        case IMAGE_RGB:
          res = shrink_row<RgbTraits>(image1, image2, x1, y, x2);
          break;

        case IMAGE_GRAYSCALE:
          res = shrink_row<GrayscaleTraits>(image1, image2, x1, y, x2);
          break;

        case IMAGE_INDEXED:
          res = shrink_row<IndexedTraits>(image1, image2, x1, y, x2);
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

} // namespace doc
