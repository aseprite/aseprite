// Aseprite
// Copyright (C) 2020-2025  Igara Studio S.A.
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_FONTS_FONT_DATA_H_INCLUDED
#define APP_FONTS_FONT_DATA_H_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "text/font.h"
#include "text/fwd.h"

#include <map>

namespace app {

// Represents a defined font in a <font> element from "data/fonts/fonts.xml" file
// and theme fonts (<font> elements from "data/extensions/aseprite-theme/theme.xml").
class FontData {
public:
  FontData(text::FontType type);
  FontData(const text::FontRef& nativeFont);

  text::FontType type() const { return m_type; }
  const std::string& name() const { return m_name; }
  const std::string& filename() const { return m_filename; }
  float defaultSize() const { return m_size; }
  bool antialias() const { return m_antialias; }
  text::FontHinting hinting() const { return m_hinting; }

  void setName(const std::string& name) { m_name = name; }
  void setFilename(const std::string& filename) { m_filename = filename; }
  void setDefaultSize(const float size) { m_size = size; }
  void setAntialias(const bool antialias) { m_antialias = antialias; }
  void setHinting(const text::FontHinting hinting) { m_hinting = hinting; }
  void setFallback(FontData* fallback, float fallbackSize)
  {
    m_fallback = fallback;
    m_fallbackSize = fallbackSize;
  }

  // Descent font metrics for sprite sheet fonts
  void setDescent(float descent) { m_descent = descent; }

  text::FontRef getFont(text::FontMgrRef& fontMgr, float size);

private:
  // Cache of loaded fonts so we avoid re-loading them.
  struct Cache {
    struct Key {
      float size;
      bool antialias : 1;
      bool hinting : 1;
      bool operator<(const Key& b) const
      {
        return size < b.size || antialias < b.antialias || hinting < b.hinting;
      }
    };
    std::map<Key, text::FontRef> fonts;
  };

  text::FontType m_type;
  std::string m_name;
  std::string m_filename;
  float m_size = 0.0f;
  bool m_antialias = false;
  text::FontHinting m_hinting = text::FontHinting::Normal;
  Cache m_cache;
  FontData* m_fallback = nullptr;
  float m_fallbackSize = 0.0f;
  float m_descent = 0.0f;
  text::FontRef m_nativeFont;

  DISABLE_COPYING(FontData);
};

} // namespace app

#endif
