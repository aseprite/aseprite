// Aseprite FreeType Wrapper
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef FT_FACE_H_INCLUDED
#define FT_FACE_H_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "ft/freetype_headers.h"
#include "gfx/rect.h"

namespace ft {

  class Face {
  public:
    Face(FT_Face face = nullptr) : m_face(face) {
    }

    ~Face() {
      if (m_face)
        FT_Done_Face(m_face);
    }

    operator FT_Face() { return m_face; }
    FT_Face operator->() { return m_face; }

    bool isValid() const {
      return (m_face != nullptr);
    }

    bool antialias() const {
      return m_antialias;
    }

    void setAntialias(bool antialias) {
      m_antialias = antialias;
    }

    void setSize(int size) {
      FT_Set_Pixel_Sizes(m_face, size, size);
    }

    template<typename Iterator,
             typename Callback>
    void forEachGlyph(Iterator first, Iterator end, Callback callback) {
      bool use_kerning = (FT_HAS_KERNING(m_face) ? true: false);

      FT_UInt prev_glyph = 0;
      int x = 0;
      for (; first != end; ++first) {
        FT_UInt glyph_index = FT_Get_Char_Index(m_face, *first);

        if (use_kerning && prev_glyph && glyph_index) {
          FT_Vector kerning;
          FT_Get_Kerning(m_face, prev_glyph, glyph_index,
                         FT_KERNING_DEFAULT, &kerning);
          x += kerning.x >> 6;
        }

        FT_Error err = FT_Load_Glyph(
          m_face, glyph_index,
          FT_LOAD_RENDER |
          FT_LOAD_NO_BITMAP |
          (m_antialias ? FT_LOAD_TARGET_NORMAL:
                         FT_LOAD_TARGET_MONO));

        if (!err) {
          callback(m_face->glyph, x);
          x += m_face->glyph->advance.x >> 6;
        }

        prev_glyph = glyph_index;
      }
    }

    template<typename Iterator>
    gfx::Rect calcTextBounds(Iterator first, Iterator end) {
      gfx::Rect bounds(0, 0, 0, 0);

      forEachGlyph(
        first, end,
        [&bounds](FT_GlyphSlot glyph, int x) {
          bounds |= gfx::Rect(x + glyph->bitmap_left,
                              -glyph->bitmap_top,
                              (int)glyph->bitmap.width,
                              (int)glyph->bitmap.rows);
        });

      return bounds;
    }

  private:
    FT_Face m_face;
    bool m_antialias;

    DISABLE_COPYING(Face);
  };

} // namespace ft

#endif
