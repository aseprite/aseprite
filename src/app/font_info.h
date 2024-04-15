// Aseprite
// Copyright (c) 2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_FONT_INFO_H_INCLUDED
#define APP_UI_FONT_INFO_H_INCLUDED
#pragma once

#include "base/convert_to.h"
#include "text/font_style.h"
#include "text/fwd.h"
#include "text/typeface.h"

#include <cmath>
#include <string>

namespace app {

  // TODO should we merge this with skin::FontData?
  class FontInfo {
  public:
    enum class Type {
      Unknown,
      Name,
      File,
      System,
    };

    static constexpr const float kDefaultSize = 0.0f;

    FontInfo(Type type = Type::Unknown,
             const std::string& name = {},
             float size = kDefaultSize,
             text::FontStyle style = text::FontStyle(),
             bool antialias = false);

    FontInfo(const FontInfo& other,
             float size,
             text::FontStyle style,
             bool antialias);

    bool isValid() const { return m_type != Type::Unknown; }
    bool useDefaultSize() const { return m_size == kDefaultSize; }

    Type type() const { return m_type; }

    // Depending on the font type this field indicates a "integrated
    // font name" (e.g. "Aseprite" or any font from <font>
    // definitions/themes), or a TTF filename, or a system font name.
    const std::string& name() const { return m_name; }

    // Visible label for this font in the UI.
    std::string title() const;
    std::string thumbnailId() const;

    float size() const { return m_size; }
    text::FontStyle style() const { return m_style; }
    bool antialias() const { return m_antialias; }

    text::TypefaceRef findTypeface(const text::FontMgrRef& fontMgr) const;

    bool operator==(const FontInfo& other) const {
      return (m_type == other.m_type &&
              m_name == other.m_name &&
              std::fabs(m_size-other.m_size) < 0.001f &&
              m_antialias == other.m_antialias);
    }

  private:
    Type m_type = Type::Unknown;
    std::string m_name;
    float m_size = kDefaultSize;
    text::FontStyle m_style;
    bool m_antialias = false;
  };

} // namespace app

namespace base {

  template<> app::FontInfo convert_to(const std::string& from);
  template<> std::string convert_to(const app::FontInfo& from);

} // namespace base

#endif
