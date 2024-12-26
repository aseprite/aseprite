// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/util/pixel_ratio.h"

#include "base/split_string.h"

#include <string>
#include <vector>

namespace base {

template<>
doc::PixelRatio convert_to(const std::string& from)
{
  std::vector<std::string> parts;
  split_string(from, parts, ":");
  doc::PixelRatio pixelRatio(1, 1);

  if (parts.size() == 2) {
    pixelRatio.w = convert_to<int>(parts[0]);
    pixelRatio.h = convert_to<int>(parts[1]);
    pixelRatio.w = std::max(1, pixelRatio.w);
    pixelRatio.h = std::max(1, pixelRatio.h);
  }

  return pixelRatio;
}

template<>
std::string convert_to(const doc::PixelRatio& from)
{
  return (convert_to<std::string>(from.w) + ":" + convert_to<std::string>(from.h));
}

} // namespace base
