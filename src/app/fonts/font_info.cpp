// Aseprite
// Copyright (c) 2024-2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/fonts/font_info.h"

#include "app/fonts/font_data.h"
#include "app/i18n/strings.h"
#include "app/pref/preferences.h"
#include "base/fs.h"
#include "base/split_string.h"
#include "fmt/format.h"
#include "text/font_mgr.h"
#include "text/font_style_set.h"

#include <cmath>
#include <cstdlib>
#include <vector>

namespace app {

FontInfo::FontInfo(Type type,
                   const std::string& name,
                   const float size,
                   const text::FontStyle style,
                   const Flags flags,
                   const text::FontHinting hinting)
  : m_type(type)
  , m_name(name)
  , m_size(size)
  , m_style(style)
  , m_flags(flags)
  , m_hinting(hinting)
{
}

FontInfo::FontInfo(const FontInfo& other,
                   const float size,
                   const text::FontStyle style,
                   const Flags flags,
                   text::FontHinting hinting)
  : m_type(other.type())
  , m_name(other.name())
  , m_size(size)
  , m_style(style)
  , m_flags(flags)
  , m_hinting(hinting)
{
}

FontInfo::FontInfo(const FontData* data, const float size)
  : m_type(Type::Unknown)
  , m_name(data->name())
  , m_size(size != 0.0f ? size : data->defaultSize())
  , m_flags(data->antialias() ? Flags::Antialias : Flags::None)
  , m_hinting(data->hinting())
{
  switch (data->type()) {
    case text::FontType::Unknown:     m_type = Type::Unknown; break;
    case text::FontType::SpriteSheet:
    case text::FontType::FreeType:    m_type = Type::File; break;
    case text::FontType::Native:      m_type = Type::System; break;
  }
}

std::string FontInfo::title() const
{
  return m_type == FontInfo::Type::File ? base::get_file_name(m_name) : m_name;
}

std::string FontInfo::thumbnailId() const
{
  switch (m_type) {
    case app::FontInfo::Type::Unknown: break;
    case app::FontInfo::Type::Name:    return m_name;
    case app::FontInfo::Type::File:    return "file=" + m_name;
    case app::FontInfo::Type::System:  return "system=" + m_name;
  }
  return std::string();
}

text::TypefaceRef FontInfo::findTypeface(const text::FontMgrRef& fontMgr) const
{
  if (m_type != Type::System)
    return nullptr;

  const text::FontStyleSetRef set = fontMgr->matchFamily(m_name);
  if (set)
    return set->matchStyle(m_style);

  return nullptr;
}

// static
FontInfo FontInfo::getFromPreferences()
{
  Preferences& pref = Preferences::instance();
  FontInfo fontInfo;

  // Old configuration
  if (!pref.textTool.fontFace().empty()) {
    fontInfo = FontInfo(
      FontInfo::Type::File,
      pref.textTool.fontFace(),
      pref.textTool.fontSize(),
      text::FontStyle(),
      pref.textTool.antialias() ? FontInfo::Flags::Antialias : FontInfo::Flags::None);
  }
  // New configuration
  if (!pref.textTool.fontInfo().empty()) {
    fontInfo = base::convert_to<FontInfo>(pref.textTool.fontInfo());
  }

  return fontInfo;
}

void FontInfo::updatePreferences()
{
  Preferences& pref = Preferences::instance();
  pref.textTool.fontInfo(base::convert_to<std::string>(*this));
  if (!pref.textTool.fontFace().empty()) {
    pref.textTool.fontFace.clearValue();
    pref.textTool.fontSize.clearValue();
    pref.textTool.antialias.clearValue();
  }
}

std::string FontInfo::humanString() const
{
  std::string result;

  switch (type()) {
    case app::FontInfo::Type::Unknown:
    case app::FontInfo::Type::Name:
    case app::FontInfo::Type::System:  result = name(); break;
    case app::FontInfo::Type::File:    result = base::get_file_name(name()); break;
  }
  result += fmt::format(" {}pt", size());
  if (!result.empty()) {
    if (style().weight() != text::FontStyle::Weight::Normal)
      result +=
        " " +
        Strings::Translate(
          fmt::format("font_style.font_weight_{}", static_cast<int>(style().weight())).c_str());
    if (style().slant() != text::FontStyle::Slant::Upright)
      result += " " + Strings::font_style_italic();
    if (antialias())
      result += " " + Strings::font_style_antialias();
    if (ligatures())
      result += " " + Strings::font_style_ligatures();
    switch (hinting()) {
      case text::FontHinting::None:   result += " " + Strings::font_style_hinting_none(); break;
      case text::FontHinting::Slight: result += " " + Strings::font_style_hinting_slight(); break;
      case text::FontHinting::Normal: break;
      case text::FontHinting::Full:   result += " " + Strings::font_style_hinting_full(); break;
    }
  }
  return result;
}

} // namespace app

namespace base {

template<>
app::FontInfo convert_to(const std::string& from)
{
  std::vector<std::string> parts;
  base::split_string(from, parts, ",");

  app::FontInfo::Type type = app::FontInfo::Type::Unknown;
  std::string name;
  float size = 0.0f;
  bool bold = false;
  bool italic = false;
  app::FontInfo::Flags flags = app::FontInfo::Flags::None;
  text::FontHinting hinting = text::FontHinting::Normal;
  text::FontStyle::Weight weight = text::FontStyle::Weight::Normal;

  if (!parts.empty()) {
    if (parts[0].compare(0, 5, "file=") == 0) {
      type = app::FontInfo::Type::File;
      name = parts[0].substr(5);
    }
    else if (parts[0].compare(0, 7, "system=") == 0) {
      type = app::FontInfo::Type::System;
      name = parts[0].substr(7);
    }
    else if (!parts[0].empty()) {
      type = app::FontInfo::Type::Name;
      name = parts[0];
    }
    for (int i = 1; i < parts.size(); ++i) {
      if (parts[i] == "antialias")
        flags |= app::FontInfo::Flags::Antialias;
      else if (parts[i] == "ligatures")
        flags |= app::FontInfo::Flags::Ligatures;
      else if (parts[i] == "bold")
        bold = true;
      else if (parts[i] == "italic")
        italic = true;
      else if (parts[i].compare(0, 5, "size=") == 0) {
        size = std::strtof(parts[i].substr(5).c_str(), nullptr);
      }
      else if (parts[i].compare(0, 8, "hinting=") == 0) {
        std::string hintingStr = parts[i].substr(8);
        if (hintingStr == "none")
          hinting = text::FontHinting::None;
        else if (hintingStr == "slight")
          hinting = text::FontHinting::Slight;
        else if (hintingStr == "normal")
          hinting = text::FontHinting::Normal;
        else if (hintingStr == "full")
          hinting = text::FontHinting::Full;
      }
      else if (parts[i].compare(0, 7, "weight=") == 0) {
        std::string weightStr = parts[i].substr(7);
        weight = static_cast<text::FontStyle::Weight>(std::atoi(weightStr.c_str()));
      }
    }
  }

  const text::FontStyle style(
    bold ? text::FontStyle::Weight::Bold : weight,
    text::FontStyle::Width::Normal,
    italic ? text::FontStyle::Slant::Italic : text::FontStyle::Slant::Upright);

  return app::FontInfo(type, name, size, style, flags, hinting);
}

template<>
std::string convert_to(const app::FontInfo& from)
{
  std::string result;
  switch (from.type()) {
    case app::FontInfo::Type::Unknown:
      // Do nothing
      break;
    case app::FontInfo::Type::Name:   result = from.name(); break;
    case app::FontInfo::Type::File:   result = "file=" + from.name(); break;
    case app::FontInfo::Type::System: result = "system=" + from.name(); break;
  }
  if (!result.empty()) {
    if (from.size() > 0.0f)
      result += fmt::format(",size={}", from.size());
    if (from.style().weight() == text::FontStyle::Weight::Bold)
      result += ",bold";
    if (from.style().slant() != text::FontStyle::Slant::Upright)
      result += ",italic";
    if (from.antialias())
      result += ",antialias";
    if (from.ligatures())
      result += ",ligatures";
    if (from.hinting() != text::FontHinting::Normal) {
      result += ",hinting=";
      switch (from.hinting()) {
        case text::FontHinting::None:   result += "none"; break;
        case text::FontHinting::Slight: result += "slight"; break;
        case text::FontHinting::Normal:
          // Filtered out by above if
          break;
        case text::FontHinting::Full: result += "full"; break;
      }
    }
    if (from.style().weight() != text::FontStyle::Weight::Bold)
      result += ",weight=" + std::to_string(static_cast<int>(from.style().weight()));
  }
  return result;
}

} // namespace base
