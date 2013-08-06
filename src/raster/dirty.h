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

#ifndef RASTER_DIRTY_H_INCLUDED
#define RASTER_DIRTY_H_INCLUDED

#include "raster/image.h"

#include <vector>

#define DIRTY_VALID_COLUMN      1
#define DIRTY_MUSTBE_UPDATED    2

namespace raster {

  class Pen;
  class Image;
  class Mask;

  class Dirty {
  public:
    struct Col;
    struct Row;

    typedef std::vector<Col*> ColsList;
    typedef std::vector<Row*> RowsList;

    struct Col {
      int x, w;
      std::vector<uint8_t> data;

      Col(int x, int w) : x(x), w(w) { }
    };

    struct Row {
      int y;
      ColsList cols;

      Row(int y) : y(y) { }
    };

    Dirty(PixelFormat format, int x1, int y1, int x2, int y2);
    Dirty(const Dirty& src);
    Dirty(Image* image1, Image* image2);
    ~Dirty();

    int getMemSize() const;

    PixelFormat getPixelFormat() const { return m_format; }
    int x1() const { return m_x1; }
    int y1() const { return m_y1; }
    int x2() const { return m_x2; }
    int y2() const { return m_y2; }

    int getRowsCount() const { return m_rows.size(); }
    const Row& getRow(int i) const { return *m_rows[i]; }

    inline int getLineSize(int width) const {
      return pixelformat_line_size(m_format, width);
    }

    void saveImagePixels(Image* image);
    void swapImagePixels(Image* image);

    Dirty* clone() const { return new Dirty(*this); }

  private:
    // Disable copying through operator=
    Dirty& operator=(const Dirty&);

    //private: // TODO these fields are public because Undo creates
  public:    // a Dirty instance from a deserialization process,
    // remember to "privatize" these members when the
    // new Undo implementation is finished.

    PixelFormat m_format;
    int m_x1, m_y1;
    int m_x2, m_y2;
    RowsList m_rows;

  };

} // namespace raster

#endif
