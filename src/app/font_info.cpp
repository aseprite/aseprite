// Aseprite
// Copyright (c) 2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/font_info.h"
#include "base/fs.h"
#include "base/split_string.h"
#include "fmt/format.h"
#include "text/font_mgr.h"
#include "text/font_style_set.h"

#include <cstdlib>
#include <cmath>
#include <vector>

namespace app {

FontInfo::FontInfo(Type type,
                   const std::string& name,
                   const float size,
                   const bool antialias,
                   const text::TypefaceRef& typeface)
  : m_type(type)
  , m_name(name)
  , m_size(size)
  , m_antialias(antialias)
  , m_typeface(typeface)
{
}

FontInfo::FontInfo(const FontInfo& other,
                   const float size,
                   const bool antialias)
  : m_type(other.type())
  , m_name(other.name())
  , m_size(size)
  , m_antialias(antialias)
  , m_typeface(other.typeface())
{
}

std::string FontInfo::title() const
{
  return m_type == FontInfo::Type::File ?
    base::get_file_name(m_name):
    m_name;
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

void FontInfo::findTypeface(const text::FontMgrRef& fontMgr) const
{
  if (m_type != Type::System ||
      m_typeface != nullptr) {
    return;
  }

  const text::FontStyleSetRef set = fontMgr->matchFamily(m_name);
  if (set && set->typeface(0))
    m_typeface = set->typeface(0);
}

} // namespace app

namespace base {

template<> app::FontInfo convert_to(const std::string& from)
{
  std::vector<std::string> parts;
  base::split_string(from, parts, ",");

  app::FontInfo::Type type = app::FontInfo::Type::Unknown;
  std::string name;
  float size = 0.0f;
  bool antialias = false;

  if (!parts.empty()) {
    if (parts[0].compare(0, 5, "file=") == 0) {
      type = app::FontInfo::Type::File;
      name = parts[0].substr(5);
    }
    else if (parts[0].compare(0, 7, "system=") == 0) {
      type = app::FontInfo::Type::System;
      name = parts[0].substr(7);
    }
    else {
      type = app::FontInfo::Type::Name;
      name = parts[0];
    }
    for (int i=1; i<parts.size(); ++i) {
      if (parts[i] == "antialias")
        antialias = true;
      else if (parts[i].compare(0, 5, "size=") == 0) {
        size = std::strtof(parts[i].substr(5).c_str(), nullptr);
      }
    }
  }
  return app::FontInfo(type, name, size, antialias);
}

template<> std::string convert_to(const app::FontInfo& from)
{
  std::string result;
  switch (from.type()) {
    case app::FontInfo::Type::Unknown:
      // Do nothing
      break;
    case app::FontInfo::Type::Name:
      result = from.name();
      break;
    case app::FontInfo::Type::File:
      result = "file=" + from.name();
      break;
    case app::FontInfo::Type::System:
      result = "system=" + from.name();
      break;
  }
  if (!result.empty()) {
    if (from.size() > 0.0f)
      result += fmt::format(",size={}", from.size());
    if (from.antialias())
      result += ",antialias";
  }
  return result;
}

} // namespace base
