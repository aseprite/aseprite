// Aseprite
// Copyright (C) 2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/fonts/fonts.h"

#include "app/fonts/font_data.h"
#include "app/fonts/font_info.h"
#include "text/font_mgr.h"

namespace app {

static Fonts* g_instance = nullptr;

// static
Fonts* Fonts::instance()
{
  ASSERT(g_instance);
  return g_instance;
}

Fonts::Fonts(const text::FontMgrRef& fontMgr) : m_fontMgr(fontMgr)
{
  ASSERT(!g_instance);
  g_instance = this;
}

Fonts::~Fonts()
{
  ASSERT(g_instance == this);
  g_instance = nullptr;
}

void Fonts::addFontData(const std::string& name, std::unique_ptr<FontData>&& fontData)
{
  m_fonts[name] = std::move(fontData);
}

FontData* Fonts::fontDataByName(const std::string& name)
{
  auto it = m_fonts.find(name);
  if (it == m_fonts.end())
    return nullptr;
  return it->second.get();
}

text::FontRef Fonts::fontByName(const std::string& name, int size)
{
  auto it = m_fonts.find(name);
  if (it == m_fonts.end())
    return nullptr;
  return it->second->getFont(m_fontMgr, size);
}

text::FontRef Fonts::fontFromInfo(const FontInfo& fontInfo)
{
  ASSERT(m_fontMgr);
  if (!m_fontMgr)
    return nullptr;

  text::FontRef font;
  if (fontInfo.type() == FontInfo::Type::System) {
    // Just in case the typeface is not present in the FontInfo
    auto typeface = fontInfo.findTypeface(m_fontMgr);

    font = m_fontMgr->makeFont(typeface);
    if (!fontInfo.useDefaultSize())
      font->setSize(fontInfo.size());
  }
  else {
    const int size = (fontInfo.useDefaultSize() ? 18 : fontInfo.size());
    font = fontByName(fontInfo.name(), size);

    if (!font && fontInfo.type() == FontInfo::Type::File) {
      font = m_fontMgr->loadTrueTypeFont(fontInfo.name().c_str(), size);
    }
  }

  if (font)
    font->setAntialias(fontInfo.antialias());

  return font;
}

} // namespace app
