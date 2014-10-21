// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_DIRTY_H_INCLUDED
#define DOC_DIRTY_H_INCLUDED
#pragma once

#include "doc/image.h"

#include <vector>

#define DIRTY_VALID_COLUMN      1
#define DIRTY_MUSTBE_UPDATED    2

namespace doc {

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

public:
    Dirty(PixelFormat format, int x1, int y1, int x2, int y2);
    Dirty(const Dirty& src);
    Dirty(Image* image1, Image* image2, const gfx::Rect& bounds);
    ~Dirty();

    int getMemSize() const;

    PixelFormat pixelFormat() const { return m_format; }
    int x1() const { return m_x1; }
    int y1() const { return m_y1; }
    int x2() const { return m_x2; }
    int y2() const { return m_y2; }

    int getRowsCount() const { return m_rows.size(); }
    const Row& getRow(int i) const { return *m_rows[i]; }

    inline int getLineSize(int width) const {
      return calculate_rowstride_bytes(m_format, width);
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

} // namespace doc

#endif
