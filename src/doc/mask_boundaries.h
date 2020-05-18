// Aseprite Document Library
// Copyright (c) 2020 Igara Studio S.A.
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_MASK_BOUNDARIES_H_INCLUDED
#define DOC_MASK_BOUNDARIES_H_INCLUDED
#pragma once

#include "gfx/path.h"
#include "gfx/rect.h"

#include <vector>

namespace doc {
  class Image;

  class MaskBoundaries {
  public:
    class Segment {
    public:
      Segment(bool open, const gfx::Rect& bounds)
        : m_open(open), m_bounds(bounds) { }

      // Returns true if this segment enters into the boundaries.
      bool open() const { return m_open; }

      const gfx::Rect& bounds() const { return m_bounds; }
      void offset(int x, int y) { m_bounds.offset(x, y); }
      bool vertical() const { return m_bounds.w == 0; }
      bool horizontal() const { return m_bounds.h == 0; }

    private:
      bool m_open;
      gfx::Rect m_bounds;

      friend class MaskBoundaries;
    };

    typedef std::vector<Segment> list_type;
    typedef list_type::iterator iterator;
    typedef list_type::const_iterator const_iterator;

    bool isEmpty() const { return m_segs.empty(); }
    void reset();
    void regen(const Image* bitmap);

    const_iterator begin() const { return m_segs.begin(); }
    const_iterator end() const { return m_segs.end(); }
    iterator begin() { return m_segs.begin(); }
    iterator end() { return m_segs.end(); }

    void offset(int x, int y);
    gfx::Path& path() { return m_path; }

    void createPathIfNeeeded();

  private:
    list_type m_segs;
    gfx::Path m_path;
  };

} // namespace doc

#endif
