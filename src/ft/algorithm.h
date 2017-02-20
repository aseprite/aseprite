// Aseprite FreeType Wrapper
// Copyright (c) 2016-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef FT_ALGORITHM_H_INCLUDED
#define FT_ALGORITHM_H_INCLUDED
#pragma once

#include "base/string.h"
#include "ft/freetype_headers.h"
#include "ft/hb_shaper.h"
#include "gfx/rect.h"

namespace ft {

  template<typename FaceFT>
  class DefaultShaper {
  public:
    typedef typename FaceFT::Glyph Glyph;

    DefaultShaper(FaceFT& face) : m_face(face) {
    }

    bool initialize(const base::utf8_const_iterator& it,
                    const base::utf8_const_iterator& end) {
      m_it = it;
      m_end = end;
      return (m_it != end);
    }

    bool nextChar() {
      ++m_it;
      return (m_it != m_end);
    }

    int unicodeChar() const {
      return *m_it;
    }

    unsigned int glyphIndex() {
      return m_face.cache().getGlyphIndex(m_face, unicodeChar());
    }

    void glyphOffsetXY(Glyph* glyph) {
      // Do nothing
    }

    void glyphAdvanceXY(const Glyph* glyph, double& x, double& y) {
      x += glyph->ft_glyph->advance.x / double(1 << 16);
      y += glyph->ft_glyph->advance.y / double(1 << 16);
    }

  private:
    FaceFT& m_face;
    base::utf8_const_iterator m_it;
    base::utf8_const_iterator m_end;
  };

  template<typename FaceFT,
           typename Shaper = HBShaper<FaceFT> >
  class ForEachGlyph {
  public:
    typedef typename FaceFT::Glyph Glyph;

    ForEachGlyph(FaceFT& face)
      : m_face(face)
      , m_shaper(face)
      , m_glyph(nullptr)
      , m_useKerning(FT_HAS_KERNING((FT_Face)face) ? true: false)
      , m_prevGlyph(0)
      , m_x(0.0), m_y(0.0)
      , m_index(0) {
    }

    ~ForEachGlyph() {
      unloadGlyph();
    }

    int unicodeChar() { return m_shaper.unicodeChar(); }

    const Glyph* glyph() const { return m_glyph; }

    bool initialize(const base::utf8_const_iterator& it,
                    const base::utf8_const_iterator& end) {
      bool res = m_shaper.initialize(it, end);
      if (res)
        prepareGlyph();
      return res;
    }

    bool nextChar() {
      m_prevGlyph = m_shaper.glyphIndex();

      if (m_shaper.nextChar()) {
        prepareGlyph();
        return true;
      }
      else
        return false;
    }

  private:
    void prepareGlyph() {
      FT_UInt glyphIndex = m_shaper.glyphIndex();
      double initialX = m_x;

      if (m_useKerning && m_prevGlyph && glyphIndex) {
        FT_Vector kerning;
        FT_Get_Kerning(m_face, m_prevGlyph, glyphIndex,
                       FT_KERNING_DEFAULT, &kerning);
        m_x += kerning.x / 64.0;
      }

      unloadGlyph();

      // Load new glyph
      m_glyph = m_face.cache().loadGlyph(m_face, glyphIndex, m_face.antialias());
      if (m_glyph) {
        m_glyph->bitmap = &FT_BitmapGlyph(m_glyph->ft_glyph)->bitmap;
        m_glyph->x = m_x
          + m_glyph->bearingX;
        m_glyph->y = m_y
          + m_face.height()
          + m_face.descender() // descender is negative
          - m_glyph->bearingY;

        m_shaper.glyphOffsetXY(m_glyph);
        m_shaper.glyphAdvanceXY(m_glyph, m_x, m_y);

        m_glyph->startX = initialX;
        m_glyph->endX = m_x;
      }
    }

    void unloadGlyph() {
      if (m_glyph) {
        m_face.cache().doneGlyph(m_glyph);
        m_glyph = nullptr;
      }
    }

  private:
    FaceFT& m_face;
    Shaper m_shaper;
    Glyph* m_glyph;
    bool m_useKerning;
    FT_UInt m_prevGlyph;
    double m_x, m_y;
    int m_index;
  };

  template<typename FaceFT>
  gfx::Rect calc_text_bounds(FaceFT& face, const std::string& str) {
    gfx::Rect bounds(0, 0, 0, 0);
    ForEachGlyph<FaceFT> feg(face);
    if (feg.initialize(base::utf8_const_iterator(str.begin()),
                       base::utf8_const_iterator(str.end()))) {
      do {
        if (auto glyph = feg.glyph())
          bounds |= gfx::Rect(int(glyph->x),
                              int(glyph->y),
                              glyph->bitmap->width,
                              glyph->bitmap->rows);
      } while (feg.nextChar());
    }
    return bounds;
  }

} // namespace ft

#endif
