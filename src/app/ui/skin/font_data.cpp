// Aseprite
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

FontData::~FontData()
{
#if _DEBUG
  static std::set<os::Font*> deletedFonts;
#endif

  // Destroy all fonts
  for (auto& it : m_fonts) {
    os::Font* font = it.second;
    if (font) {
#if _DEBUG // Check that there are not double-cached fonts
      auto it2 = deletedFonts.find(font);
      ASSERT(it2 == deletedFonts.end());
      deletedFonts.insert(font);
#endif
      // Don't delete font->fallback() as it's already m_fonts
      font->dispose();
    }
  }
  m_fonts.clear();
}

os::Font* FontData::getFont(int size)
{
  if (m_type == os::FontType::kSpriteSheet)
    size = 1;                   // Same size always

  // Use cache
  size *= ui::guiscale();
  auto it = m_fonts.find(size);
  if (it != m_fonts.end())
    return it->second;

  os::Font* font = nullptr;

  switch (m_type) {
    case os::FontType::kSpriteSheet:
      font = os::instance()->loadSpriteSheetFont(m_filename.c_str(), size);
      break;
    case os::FontType::kTrueType: {
      font = os::instance()->loadTrueTypeFont(m_filename.c_str(), size);
      if (font)
        font->setAntialias(m_antialias);
      break;
    }
  }

  if (m_fallback) {
    os::Font* fallback = m_fallback->getFont(m_fallbackSize);
    if (font)
      font->setFallback(fallback);
    else
      return fallback;          // Don't double-cache the fallback font
  }

  // Cache this font
  m_fonts[size] = font;
  return font;
}

} // namespace skin
} // namespace app
