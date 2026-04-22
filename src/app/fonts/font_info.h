// Aseprite
// Copyright (c) 2024-2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_FONTS_FONT_INFO_H_INCLUDED
#define APP_FONTS_FONT_INFO_H_INCLUDED
#pragma once

#include "base/convert_to.h"
#include "base/enum_flags.h"
#include "text/font_hinting.h"
#include "text/font_style.h"
#include "text/font_type.h"
#include "text/fwd.h"
#include "text/typeface.h"

#include <cmath>
#include <string>

namespace app {

class FontData;

// Represents a font reference from any place:
// - Name: a font referenced by name, a font that came from fonts.xml files (Fonts/FontData)
// - File: an external font loaded from a .ttf file
// - System: native laf-os fonts (i.e. Skia fonts loaded from the operating system)
//
// This font reference can be serialize to a string to be saved in the
// aseprite.ini configuration (e.g. latest font used in text tool, or
// custom theme fonts, etc.).
class FontInfo {
public:
  enum class Type {
    Unknown,
    Name,
    File,
    System,
  };

  enum class Flags {
    None = 0,
    Antialias = 1,
    Ligatures = 2,
  };

  static constexpr const float kDefaultSize = 0.0f;
  static constexpr const float kMinSize = 0.0f;
  static constexpr const float kMaxSize = 1000.0f;

  FontInfo(Type type = Type::Unknown,
           const std::string& name = {},
           float size = kDefaultSize,
           text::FontStyle style = text::FontStyle(),
           Flags flags = Flags::None,
           text::FontHinting hinting = text::FontHinting::Normal);

  FontInfo(const FontInfo& other,
           float size,
           text::FontStyle style,
           Flags flags,
           text::FontHinting hinting);

  FontInfo(const FontData* data, float size = 0.0f);

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
  Flags flags() const { return m_flags; }
  bool antialias() const;
  bool ligatures() const;
  text::FontHinting hinting() const { return m_hinting; }

  text::TypefaceRef findTypeface(const text::FontMgrRef& fontMgr) const;

  static FontInfo getFromPreferences();
  void updatePreferences();

  std::string humanString() const;

  bool operator==(const FontInfo& other) const
  {
    return (m_type == other.m_type && m_name == other.m_name &&
            std::fabs(m_size - other.m_size) < 0.001f && m_flags == other.m_flags);
  }

private:
  Type m_type = Type::Unknown;
  std::string m_name;
  float m_size = kDefaultSize;
  text::FontStyle m_style;
  Flags m_flags = Flags::None;
  text::FontHinting m_hinting = text::FontHinting::Normal;
};

LAF_ENUM_FLAGS(FontInfo::Flags);

inline bool FontInfo::antialias() const
{
  return (m_flags & Flags::Antialias) == Flags::Antialias;
}

inline bool FontInfo::ligatures() const
{
  return (m_flags & Flags::Ligatures) == Flags::Ligatures;
}

} // namespace app

namespace base {

template<>
app::FontInfo convert_to(const std::string& from);
template<>
std::string convert_to(const app::FontInfo& from);

} // namespace base

#endif
