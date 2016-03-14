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

#include <map>

namespace ft {

  template<typename Cache>
  class FaceT {
  public:
    FaceT(FT_Face face = nullptr) : m_face(face) {
    }

    ~FaceT() {
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
      m_cache.invalidate();
    }

    void setSize(int size) {
      FT_Set_Pixel_Sizes(m_face, size, size);
      m_cache.invalidate();
    }

    template<typename Iterator,
             typename Callback>
    void forEachGlyph(Iterator first, Iterator end, Callback callback) {
      bool use_kerning = (FT_HAS_KERNING(m_face) ? true: false);

      FT_UInt prev_glyph = 0;
      int x = 0;
      for (; first != end; ++first) {
        FT_UInt glyph_index = m_cache.getGlyphIndex(m_face, *first);

        if (use_kerning && prev_glyph && glyph_index) {
          FT_Vector kerning;
          FT_Get_Kerning(m_face, prev_glyph, glyph_index,
                         FT_KERNING_DEFAULT, &kerning);
          x += kerning.x >> 6;
        }

        FT_Glyph glyph = m_cache.loadGlyph(m_face, glyph_index, m_antialias);
        if (glyph) {
          callback((FT_BitmapGlyph)glyph, x);
          x += glyph->advance.x >> 16;
          m_cache.doneGlyph(glyph);
        }

        prev_glyph = glyph_index;
      }
    }

    template<typename Iterator>
    gfx::Rect calcTextBounds(Iterator first, Iterator end) {
      gfx::Rect bounds(0, 0, 0, 0);

      forEachGlyph(
        first, end,
        [&bounds](FT_BitmapGlyph glyph, int x) {
          bounds |= gfx::Rect(x + glyph->left,
                              -glyph->top,
                              glyph->bitmap.width,
                              glyph->bitmap.rows);
        });

      return bounds;
    }

  private:
    FT_Face m_face;
    bool m_antialias;
    Cache m_cache;

    DISABLE_COPYING(FaceT);
  };

  class NoCache {
  public:
    void invalidate() {
      // Do nothing
    }

    FT_UInt getGlyphIndex(FT_Face face, int charCode) {
      return FT_Get_Char_Index(face, charCode);
    }

    FT_Glyph loadGlyph(FT_Face face, FT_UInt glyphIndex, bool antialias) {
      FT_Error err = FT_Load_Glyph(
        face, glyphIndex,
        FT_LOAD_RENDER |
        (antialias ? FT_LOAD_TARGET_NORMAL:
                     FT_LOAD_TARGET_MONO));
      if (err)
        return nullptr;

      FT_Glyph glyph;
      err = FT_Get_Glyph(face->glyph, &glyph);
      if (err)
        return nullptr;

      if (glyph->format != FT_GLYPH_FORMAT_BITMAP) {
        err = FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, 0, 1);
        if (!err) {
          FT_Done_Glyph(glyph);
          return nullptr;
        }
      }

      return glyph;
    }

    void doneGlyph(FT_Glyph glyph) {
      FT_Done_Glyph(glyph);
    }

  };

  class SimpleCache : public NoCache {
  public:
    ~SimpleCache() {
      invalidate();
    }

    void invalidate() {
      for (auto& it : m_glyphMap)
        FT_Done_Glyph(it.second);

      m_glyphMap.clear();
    }

    FT_Glyph loadGlyph(FT_Face face, FT_UInt glyphIndex, bool antialias) {
      auto it = m_glyphMap.find(glyphIndex);
      if (it != m_glyphMap.end())
        return it->second;

      FT_Glyph glyph = NoCache::loadGlyph(face, glyphIndex, antialias);
      if (glyph) {
        FT_Glyph newGlyph = nullptr;
        FT_Glyph_Copy(glyph, &newGlyph);
        if (newGlyph) {
          m_glyphMap[glyphIndex] = newGlyph;
          FT_Done_Glyph(glyph);
          return newGlyph;
        }
      }
      return glyph;
    }

    void doneGlyph(FT_Glyph glyph) {
      // Do nothing
    }

  private:
    std::map<FT_UInt, FT_Glyph> m_glyphMap;
  };

  typedef FaceT<SimpleCache> Face;

} // namespace ft

#endif
