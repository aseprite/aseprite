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
  static std::string str = "Tgjp";
  return m_face.calcTextBounds(str.begin(), str.end()).h;
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
