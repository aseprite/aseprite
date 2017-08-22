// SHE library
// Copyright (C) 2016-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/common/freetype_font.h"

#include "base/string.h"
#include "ft/algorithm.h"
#include "gfx/point.h"
#include "gfx/size.h"

namespace she {

FreeTypeFont::FreeTypeFont(ft::Lib& lib,
                           const char* filename,
                           const int height)
  : m_face(lib.open(filename))
{
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
  return int(m_face.height());
}

int FreeTypeFont::textLength(const std::string& str) const
{
  return ft::calc_text_bounds(m_face, str).w;
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

bool FreeTypeFont::hasCodePoint(int codepoint) const
{
  return m_face.hasCodePoint(codepoint);
}

FreeTypeFont* load_free_type_font(ft::Lib& lib,
                                  const char* filename,
                                  const int height)
{
  FreeTypeFont* font = new FreeTypeFont(lib, filename, height);
  if (!font->isValid()) {
    delete font;
    font = nullptr;
  }
  return font;
}

} // namespace she
