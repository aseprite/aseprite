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

// TODO should we merge this with FontInfo?
class FontData {
public:
  FontData(text::FontType type);

  void setFilename(const std::string& filename) { m_filename = filename; }
  void setAntialias(bool antialias) { m_antialias = antialias; }
  void setFallback(FontData* fallback, int fallbackSize)
  {
    m_fallback = fallback;
    m_fallbackSize = fallbackSize;
  }

  text::FontRef getFont(text::FontMgrRef& fontMgr, int size, int uiscale);
  text::FontRef getFont(text::FontMgrRef& fontMgr, int size);

  const std::string& filename() const { return m_filename; }

private:
  text::FontType m_type;
  std::string m_filename;
  bool m_antialias;
  std::map<int, text::FontRef> m_fonts; // key=font size, value=real font
  FontData* m_fallback;
  int m_fallbackSize;

  DISABLE_COPYING(FontData);
};

} // namespace app

#endif
