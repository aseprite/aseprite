// SHE library
// Copyright (C) 2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/common/freetype_font.h"

#include "gfx/point.h"
#include "gfx/size.h"

namespace she {

FreeTypeFont::FreeTypeFont(const char* filename, int height)
  : m_face(m_ft.open(filename))
{
  ASSERT(m_face.isValid());
  if (m_face.isValid())
    m_face.setSize(height);
}

FreeTypeFont::~FreeTypeFont()
{
}

bool FreeTypeFont::isValid() const
{
  return m_face.isValid();
}

void FreeTypeFont::dispose()
{
  delete this;
}

FontType FreeTypeFont::type()
{
  return FontType::kTrueType;
}

int FreeTypeFont::height() const
{
  FT_UInt glyph_index = FT_Get_Char_Index(m_face, 'A');

  FT_Error err = FT_Load_Glyph(
    m_face, glyph_index,
    FT_LOAD_RENDER |
    FT_LOAD_NO_BITMAP |
    (m_face.antialias() ? FT_LOAD_TARGET_NORMAL:
                          FT_LOAD_TARGET_MONO));

  if (!err)
    return (int)m_face->glyph->bitmap.rows;
  else
    return m_face->height >> 6;
}

int FreeTypeFont::charWidth(int chr) const
{
  return m_face.calcTextBounds(&chr, (&chr)+1).w;
}

int FreeTypeFont::textLength(const std::string& str) const
{
  return m_face.calcTextBounds(str.begin(), str.end()).w;
}

bool FreeTypeFont::isScalable() const
{
  return true;
}

void FreeTypeFont::setSize(int size)
{
  m_face.setSize(size);
}

void FreeTypeFont::setAntialias(bool antialias)
{
  m_face.setAntialias(antialias);
}

FreeTypeFont* loadFreeTypeFont(const char* filename, int height)
{
  FreeTypeFont* font = new FreeTypeFont(filename, height);
  if (!font->isValid()) {
    delete font;
    font = nullptr;
  }
  return font;
}

} // namespace she
