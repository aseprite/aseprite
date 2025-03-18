// Aseprite
// Copyright (C) 2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_FONTS_FONTS_H_INCLUDED
#define APP_FONTS_FONTS_H_INCLUDED
#pragma once

#include "text/fwd.h"

#include <map>
#include <memory>
#include <string>

namespace app {

class FontData;
using FontDataMap = std::map<std::string, std::unique_ptr<FontData>>;
class FontInfo;

// Available defined fonts in "data/fonts/fonts.xml" file and theme
// fonts (<font> elements from "data/extensions/aseprite-theme/theme.xml").
class Fonts {
public:
  static Fonts* instance();

  Fonts(const text::FontMgrRef& fontMgr);
  ~Fonts();

  const text::FontMgrRef& fontMgr() const { return m_fontMgr; }
  const FontDataMap& definedFonts() const { return m_fonts; }
  bool isEmpty() const { return m_fonts.empty(); }

  void addFontData(const std::string& name, std::unique_ptr<FontData>&& fontData);

  FontData* fontDataByName(const std::string& name);
  text::FontRef fontByName(const std::string& name, int size);
  text::FontRef fontFromInfo(const FontInfo& fontInfo);
  FontInfo infoFromFont(const text::FontRef& font);

private:
  text::FontMgrRef m_fontMgr;
  FontDataMap m_fonts;
};

} // namespace app

#endif
