// Aseprite FreeType Wrapper
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef FT_FACE_H_INCLUDED
#define FT_FACE_H_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "base/string.h"
#include "ft/freetype_headers.h"
#include "gfx/rect.h"

#include <map>

namespace ft {

  template<typename Cache>
  class FaceBase {
  public:
    struct Glyph {
      FT_UInt glyph_index;
      FT_Bitmap* bitmap;
      double x;
      double y;
    };

    FaceBase(FT_Face face) : m_face(face) {
    }

    ~FaceBase() {
      if (m_face)
        FT_Done_Face(m_face);
    }

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

  protected:
    FT_Face m_face;
    bool m_antialias;
    Cache m_cache;

  private:
    DISABLE_COPYING(FaceBase);
  };

  template<typename Cache>
  class FaceFT : public FaceBase<Cache> {
  public:
    using FaceBase<Cache>::m_face;
    using FaceBase<Cache>::m_cache;

    FaceFT(FT_Face face)
      : FaceBase<Cache>(face) {
    }

    template<typename Callback>
    void forEachGlyph(const std::string& str, Callback callback) {
      bool use_kerning = (FT_HAS_KERNING(m_face) ? true: false);
      FT_UInt prev_glyph = 0;
      double x = 0, y = 0;

      auto it = base::utf8_const_iterator(str.begin());
      auto end = base::utf8_const_iterator(str.end());
      for (; it != end; ++it) {
        FT_UInt glyph_index = m_cache.getGlyphIndex(m_face, *it);

        if (use_kerning && prev_glyph && glyph_index) {
          FT_Vector kerning;
          FT_Get_Kerning(m_face, prev_glyph, glyph_index,
                         FT_KERNING_DEFAULT, &kerning);
          x += kerning.x / 64.0;
        }

        FT_Glyph bitmapGlyph = m_cache.loadGlyph(m_face, glyph_index, m_antialias);
        if (bitmapGlyph) {
          Glyph glyph;
          glyph.glyph_index = glyph_index;
          glyph.bitmap = &FT_BitmapGlyph(bitmapGlyph)->bitmap;
          glyph.x = x + FT_BitmapGlyph(bitmapGlyph)->left;
          glyph.y = y - FT_BitmapGlyph(bitmapGlyph)->top;

          callback(glyph);

          x += bitmapGlyph->advance.x / double(1 << 16);
          y += bitmapGlyph->advance.y / double(1 << 16);

          m_cache.doneGlyph(bitmapGlyph);
        }

        prev_glyph = glyph_index;
      }
    }

    gfx::Rect calcTextBounds(const std::string& str) {
      gfx::Rect bounds(0, 0, 0, 0);

      forEachGlyph(
        str,
        [&bounds](Glyph& glyph) {
          bounds |= gfx::Rect(int(glyph.x),
                              int(glyph.y),
                              glyph.bitmap->width,
                              glyph.bitmap->rows);
        });

      return bounds;
    }
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

  typedef FaceFT<SimpleCache> Face;

} // namespace ft

#endif
