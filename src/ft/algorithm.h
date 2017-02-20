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
#include "gfx/rect.h"

namespace ft {

  template<typename FaceFT>
  class ForEachGlyph {
  public:
    ForEachGlyph(FaceFT& face)
      : m_face(face)
      , m_useKerning(FT_HAS_KERNING((FT_Face)face) ? true: false)
      , m_prevGlyph(0)
      , m_x(0.0), m_y(0.0)
    {
    }

    template<typename ProcessGlyph>
    void processChar(const int chr, ProcessGlyph processGlyph) {
      FT_UInt glyphIndex = m_face.cache().getGlyphIndex(m_face, chr);
      double initialX = m_x;

      if (m_useKerning && m_prevGlyph && glyphIndex) {
        FT_Vector kerning;
        FT_Get_Kerning(m_face, m_prevGlyph, glyphIndex,
                       FT_KERNING_DEFAULT, &kerning);
        m_x += kerning.x / 64.0;
      }

      typename FaceFT::Glyph* glyph = m_face.cache().loadGlyph(
        m_face, glyphIndex, m_face.antialias());
      if (glyph) {
        glyph->bitmap = &FT_BitmapGlyph(glyph->ft_glyph)->bitmap;
        glyph->x = m_x + glyph->bearingX;
        glyph->y = m_y
          + m_face.height()
          + m_face.descender() // descender is negative
          - glyph->bearingY;

        m_x += glyph->ft_glyph->advance.x / double(1 << 16);
        m_y += glyph->ft_glyph->advance.y / double(1 << 16);

        glyph->startX = initialX;
        glyph->endX = m_x;

        processGlyph(*glyph);

        m_face.cache().doneGlyph(glyph);
      }

      m_prevGlyph = glyphIndex;
    }

  private:
    FaceFT& m_face;
    bool m_useKerning;
    FT_UInt m_prevGlyph;
    double m_x, m_y;
  };

  template<typename FaceFT>
  gfx::Rect calc_text_bounds(FaceFT& face, const std::string& str) {
    gfx::Rect bounds(0, 0, 0, 0);
    auto it = base::utf8_const_iterator(str.begin());
    auto end = base::utf8_const_iterator(str.end());
    ForEachGlyph<FaceFT> feg(face);
    for (; it != end; ++it) {
      feg.processChar(
        *it,
        [&bounds](typename FaceFT::Glyph& glyph) {
          bounds |= gfx::Rect(int(glyph.x),
                              int(glyph.y),
                              glyph.bitmap->width,
                              glyph.bitmap->rows);
        });
    }
    return bounds;
  }

} // namespace ft

#endif
