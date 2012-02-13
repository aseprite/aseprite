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

#include "raster/dirty_io.h"

#include "base/serialization.h"
#include "base/unique_ptr.h"
#include "raster/dirty.h"

#include <iostream>

namespace raster {

using namespace base::serialization;
using namespace base::serialization::little_endian;

// Serialized Dirty data:
//
//    BYTE              image type
//    WORD[4]           x1, y1, x2, y2
//    WORD              rows
//    for each row
//      WORD[2]         y, columns
//      for each column
//       WORD[2]        x, w
//       for each pixel ("w" times)
//         BYTE[4]      for RGB images, or
//         BYTE[2]      for Grayscale images, or
//         BYTE         for Indexed images

void write_dirty(std::ostream& os, Dirty* dirty)
{
  write8(os, dirty->getPixelFormat());
  write16(os, dirty->x1());
  write16(os, dirty->y1());
  write16(os, dirty->x2());
  write16(os, dirty->y2());
  write16(os, dirty->getRowsCount());

  for (int v=0; v<dirty->getRowsCount(); v++) {
    const Dirty::Row& row = dirty->getRow(v);

    write16(os, row.y);
    write16(os, row.cols.size());

    for (size_t u=0; u<row.cols.size(); u++) {
      write16(os, row.cols[u]->x);
      write16(os, row.cols[u]->w);

      size_t size = dirty->getLineSize(row.cols[u]->w);
      os.write((char*)&row.cols[u]->data[0], size);
    }
  }
}

Dirty* read_dirty(std::istream& is)
{
  int u, v, x, y, w;
  int pixelFormat = read8(is);
  int x1 = read16(is);
  int y1 = read16(is);
  int x2 = read16(is);
  int y2 = read16(is);
  UniquePtr<Dirty> dirty(new Dirty(static_cast<PixelFormat>(pixelFormat), x1, y1, x2, y2));

  int noRows = read16(is);
  if (noRows > 0) {
    dirty->m_rows.resize(noRows);

    for (v=0; v<dirty->getRowsCount(); v++) {
      y = read16(is);

      UniquePtr<Dirty::Row> row(new Dirty::Row(y));

      int noCols = read16(is);
      row->cols.resize(noCols);

      for (u=0; u<noCols; u++) {
        x = read16(is);
        w = read16(is);

        UniquePtr<Dirty::Col> col(new Dirty::Col(x, w));

        int size = dirty->getLineSize(col->w);
        ASSERT(size > 0);

        col->data.resize(size);
        is.read((char*)&col->data[0], size);

        row->cols[u] = col.release();
      }

      dirty->m_rows[v] = row.release();
    }
  }

  return dirty.release();
}

}
