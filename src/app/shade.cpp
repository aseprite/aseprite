// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/shade.h"

#include "base/split_string.h"

namespace app {

Shade shade_from_string(const std::string& str)
{
  Shade shade;
  std::vector<std::string> parts;
  base::split_string(str, parts, " ");
  for (const auto& part : parts) {
    auto color = app::Color::fromString(part);
    if (color.getType() == app::Color::IndexType)
      shade.push_back(color);
  }
  return shade;
}

std::string shade_to_string(const Shade& shade)
{
  std::string res;
  for (const auto& s : shade) {
    if (!res.empty())
      res += " ";
    res += s.toString();
  }
  return res;
}

} // namespace app
