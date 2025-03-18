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
#include "ui/manager.h"
#include "ui/scale.h"

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

  // Use cache
  if (m_antialias) {
    auto it = m_antialiasFonts.find(size);
    if (it != m_antialiasFonts.end())
      return it->second;
  }
  else {
    auto it = m_fonts.find(size);
    if (it != m_fonts.end())
      return it->second;
  }

  text::FontRef font = nullptr;

  switch (m_type) {
    case text::FontType::SpriteSheet:
      font = fontMgr->loadSpriteSheetFont(m_filename.c_str(), size);
      break;
    case text::FontType::FreeType: {
      font = fontMgr->loadTrueTypeFont(m_filename.c_str(), size);
      if (font)
        font->setAntialias(m_antialias);
      break;
    }
  }

  if (m_fallback) {
    text::FontRef fallback = m_fallback->getFont(fontMgr, m_fallbackSize);
    if (font)
      font->setFallback(fallback.get());
    else
      return fallback; // Don't double-cache the fallback font
  }

  // Cache this font
  if (m_antialias)
    m_antialiasFonts[size] = font;
  else
    m_fonts[size] = font;
  return font;
}

} // namespace app
