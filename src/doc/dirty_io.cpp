// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/dirty_io.h"

#include "base/serialization.h"
#include "base/unique_ptr.h"
#include "doc/dirty.h"

#include <iostream>

namespace doc {

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
  write8(os, dirty->pixelFormat());
  write16(os, dirty->bounds().x);
  write16(os, dirty->bounds().y);
  write16(os, dirty->bounds().w);
  write16(os, dirty->bounds().h);
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
  int pixelFormat = read8(is);
  int x = read16(is);
  int y = read16(is);
  int w = read16(is);
  int h = read16(is);
  base::UniquePtr<Dirty> dirty(new Dirty(
      static_cast<PixelFormat>(pixelFormat),
      gfx::Rect(x, y, w, h)));

  int noRows = read16(is);
  if (noRows > 0) {
    dirty->m_rows.resize(noRows);

    for (int v=0; v<dirty->getRowsCount(); v++) {
      y = read16(is);

      base::UniquePtr<Dirty::Row> row(new Dirty::Row(y));

      int noCols = read16(is);
      row->cols.resize(noCols);

      for (int u=0; u<noCols; u++) {
        x = read16(is);
        w = read16(is);

        base::UniquePtr<Dirty::Col> col(new Dirty::Col(x, w));

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
