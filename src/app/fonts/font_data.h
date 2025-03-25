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

  void setFilename(const std::string& filename) { m_filename = filename; }
  void setAntialias(bool antialias) { m_antialias = antialias; }
  void setFallback(FontData* fallback, float fallbackSize)
  {
    m_fallback = fallback;
    m_fallbackSize = fallbackSize;
  }

  // Descent font metrics for sprite sheet fonts
  void setDescent(float descent) { m_descent = descent; }

  text::FontRef getFont(text::FontMgrRef& fontMgr, float size);

  const std::string& filename() const { return m_filename; }

private:
  text::FontType m_type;
  std::string m_filename;
  bool m_antialias;
  std::map<float, text::FontRef> m_fonts; // key=font size, value=real font
  std::map<float, text::FontRef> m_antialiasFonts;
  FontData* m_fallback;
  float m_fallbackSize;
  float m_descent = 0.0f;

  DISABLE_COPYING(FontData);
};

} // namespace app

#endif
