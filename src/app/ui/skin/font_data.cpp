// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/skin/font_data.h"

#include "os/font.h"
#include "os/system.h"
#include "ui/scale.h"

#include <set>

namespace app {
namespace skin {

FontData::FontData(os::FontType type)
  : m_type(type)
  , m_antialias(false)
  , m_fallback(nullptr)
  , m_fallbackSize(0)
{
}

os::FontRef FontData::getFont(int size, int uiscale)
{
    if (m_type == os::FontType::SpriteSheet)
    size = 1;                   // Same size always

  // Use cache
  size *= uiscale;
  auto it = m_fonts.find(size);
  if (it != m_fonts.end())
    return it->second;

  os::FontRef font = nullptr;

  switch (m_type) {
    case os::FontType::SpriteSheet:
      font = os::instance()->loadSpriteSheetFont(m_filename.c_str(), size);
      break;
    case os::FontType::FreeType: {
      font = os::instance()->loadTrueTypeFont(m_filename.c_str(), size);
      if (font)
        font->setAntialias(m_antialias);
      break;
    }
  }

  if (m_fallback) {
    os::FontRef fallback = m_fallback->getFont(m_fallbackSize);
    if (font)
      font->setFallback(fallback.get());
    else
      return fallback;          // Don't double-cache the fallback font
  }

  // Cache this font
  m_fonts[size] = font;
  return font;
}

os::FontRef FontData::getFont(int size)
{
  return getFont(size, ui::guiscale());
}

} // namespace skin
} // namespace app
