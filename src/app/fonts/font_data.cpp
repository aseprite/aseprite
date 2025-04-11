// Aseprite
// Copyright (C) 2020-2025  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/fonts/font_data.h"

#include "text/font.h"
#include "text/font_mgr.h"
#include "text/sprite_sheet_font.h"
#include "ui/manager.h"

#include <set>

namespace app {

FontData::FontData(text::FontType type)
  : m_type(type)
  , m_antialias(false)
  , m_fallback(nullptr)
  , m_fallbackSize(0)
{
}

text::FontRef FontData::getFont(text::FontMgrRef& fontMgr, float size)
{
  ASSERT(fontMgr);

  // Use cached fonts
  const Cache::Key cacheKey{ size, m_antialias, m_hinting != text::FontHinting::None };
  auto it = m_cache.fonts.find(cacheKey);
  if (it != m_cache.fonts.end())
    return it->second; // Cache hit

  text::FontRef font = nullptr;

  switch (m_type) {
    case text::FontType::SpriteSheet:
      font = fontMgr->loadSpriteSheetFont(m_filename.c_str(), size);
      if (font && m_descent != 0.0f) {
        static_cast<text::SpriteSheetFont*>(font.get())->setDescent(m_descent);
      }
      break;
    case text::FontType::FreeType: {
      font = fontMgr->loadTrueTypeFont(m_filename.c_str(), size);
      if (font) {
        font->setAntialias(m_antialias);
        font->setHinting(m_hinting);
      }
      break;
    }
  }

  // Cache this font
  m_cache.fonts[cacheKey] = font;

  // Load fallback
  if (m_fallback) {
    text::FontRef fallback = m_fallback->getFont(fontMgr, m_fallbackSize);
    if (font)
      font->setFallback(fallback.get());
    else
      return fallback; // Don't double-cache the fallback font
  }
  return font;
}

} // namespace app
