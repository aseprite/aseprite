// Aseprite FreeType Wrapper
// Copyright (c) 2016-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef FT_FACE_H_INCLUDED
#define FT_FACE_H_INCLUDED
#pragma once

#include "base/debug.h"
#include "base/disable_copying.h"
#include "base/string.h"
#include "ft/freetype_headers.h"
#include "gfx/rect.h"

#include <map>

namespace ft {

  struct Glyph {
    FT_UInt glyph_index;
    FT_Glyph ft_glyph;
    FT_Bitmap* bitmap;
    double startX;
    double endX;
    double bearingX;
    double bearingY;
    double x;
    double y;
  };

  template<typename Cache>
  class FaceFT {
  public:
    FaceFT(FT_Face face) : m_face(face) {
    }

    ~FaceFT() {
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

    double height() const {
      FT_Size_Metrics* metrics = &m_face->size->metrics;
      double em_size = 1.0 * m_face->units_per_EM;
      double y_scale = metrics->y_ppem / em_size;
      return int(m_face->height * y_scale) - 1;
    }

    double ascender() const {
      FT_Size_Metrics* metrics = &m_face->size->metrics;
      double em_size = 1.0 * m_face->units_per_EM;
      double y_scale = metrics->y_ppem / em_size;
      return int(m_face->ascender * y_scale);
    }

    double descender() const {
      FT_Size_Metrics* metrics = &m_face->size->metrics;
      double em_size = 1.0 * m_face->units_per_EM;
      double y_scale = metrics->y_ppem / em_size;
      return int(m_face->descender * y_scale);
    }

    Cache& cache() { return m_cache; }

  protected:
    FT_Face m_face;
    bool m_antialias;
    Cache m_cache;

  private:
    DISABLE_COPYING(FaceFT);
  };

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

      Glyph* glyph = m_face.cache().loadGlyph(
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
        [&bounds](Glyph& glyph) {
          bounds |= gfx::Rect(int(glyph.x),
                              int(glyph.y),
                              glyph.bitmap->width,
                              glyph.bitmap->rows);
        });
    }
    return bounds;
  }

  class NoCache {
  public:
    void invalidate() {
      // Do nothing
    }

    FT_UInt getGlyphIndex(FT_Face face, int charCode) {
      return FT_Get_Char_Index(face, charCode);
    }

    Glyph* loadGlyph(FT_Face face, FT_UInt glyphIndex, bool antialias) {
      FT_Error err = FT_Load_Glyph(
        face, glyphIndex,
        FT_LOAD_RENDER |
        (antialias ? FT_LOAD_TARGET_NORMAL:
                     FT_LOAD_TARGET_MONO));
      if (err)
        return nullptr;

      FT_Glyph ft_glyph;
      err = FT_Get_Glyph(face->glyph, &ft_glyph);
      if (err)
        return nullptr;

      if (ft_glyph->format != FT_GLYPH_FORMAT_BITMAP) {
        err = FT_Glyph_To_Bitmap(&ft_glyph, FT_RENDER_MODE_NORMAL, 0, 1);
        if (!err) {
          FT_Done_Glyph(ft_glyph);
          return nullptr;
        }
      }

      m_glyph.ft_glyph = ft_glyph;
      m_glyph.bearingX = face->glyph->metrics.horiBearingX / 64.0;
      m_glyph.bearingY = face->glyph->metrics.horiBearingY / 64.0;

      return &m_glyph;
    }

    void doneGlyph(Glyph* glyph) {
      ASSERT(glyph);
      FT_Done_Glyph(glyph->ft_glyph);
    }

  private:
    Glyph m_glyph;
  };

  class SimpleCache : public NoCache {
  public:
    ~SimpleCache() {
      invalidate();
    }

    void invalidate() {
      for (auto& it : m_glyphMap) {
        FT_Done_Glyph(it.second->ft_glyph);
        delete it.second;
      }

      m_glyphMap.clear();
    }

    Glyph* loadGlyph(FT_Face face, FT_UInt glyphIndex, bool antialias) {
      auto it = m_glyphMap.find(glyphIndex);
      if (it != m_glyphMap.end())
        return it->second;

      Glyph* glyph = NoCache::loadGlyph(face, glyphIndex, antialias);
      if (!glyph)
        return nullptr;

      FT_Glyph new_ft_glyph = nullptr;
      FT_Glyph_Copy(glyph->ft_glyph, &new_ft_glyph);
      if (!new_ft_glyph)
        return nullptr;

      Glyph* newGlyph = new Glyph(*glyph);
      newGlyph->ft_glyph = new_ft_glyph;

      m_glyphMap[glyphIndex] = newGlyph;
      FT_Done_Glyph(glyph->ft_glyph);

      return newGlyph;
    }

    void doneGlyph(Glyph* glyph) {
      // Do nothing
    }

  private:
    std::map<FT_UInt, Glyph*> m_glyphMap;
  };

  typedef FaceFT<SimpleCache> Face;

} // namespace ft

#endif
