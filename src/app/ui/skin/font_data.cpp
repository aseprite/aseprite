// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/skin/font_data.h"

#include "she/font.h"
#include "she/system.h"

namespace app {
namespace skin {

FontData::FontData(she::FontType type)
  : m_type(type)
  , m_antialias(false)
  , m_fallback(nullptr)
  , m_fallbackSize(0)
{
}

FontData::~FontData()
{
  // Destroy all fonts
  for (auto& it : m_fonts) {
    she::Font* font = it.second;
    if (font) {
      if (font->fallback())
        font->fallback()->dispose();
      font->dispose();
    }
  }
}

she::Font* FontData::getFont(int size, bool useCache)
{
  if (m_type == she::FontType::kSpriteSheet)
    size = 0;                   // Same size always

  if (useCache) {
    auto it = m_fonts.find(size);
    if (it != m_fonts.end())
      return it->second;
  }

  she::Font* font = nullptr;

  switch (m_type) {
    case she::FontType::kSpriteSheet:
      font = she::instance()->loadSpriteSheetFont(m_filename.c_str());
      break;
    case she::FontType::kTrueType:
      font = she::instance()->loadTrueTypeFont(m_filename.c_str(), size);
      if (font)
        font->setAntialias(m_antialias);
      break;
  }

  if (m_fallback) {
    she::Font* fallback = m_fallback->getFont(
      m_fallbackSize,
      false); // Do not use cache

    if (font)
      font->setFallback(fallback);
    else
      font = fallback;
  }

  if (useCache)
    m_fonts[size] = font;

  return font;
}

} // namespace skin
} // namespace app
