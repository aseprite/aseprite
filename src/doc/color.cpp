// Aseprite Document Library
// Copyright (c) 2020 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/color.h"

#include <algorithm>

namespace doc {

color_t rgba_to_graya_using_hsv(const color_t c)
{
  const uint8_t M = std::max(rgba_getr(c),
                             std::max(rgba_getg(c),
                                      rgba_getb(c)));
  return graya(M,
               rgba_geta(c));
}

color_t rgba_to_graya_using_hsl(const color_t c)
{
  const int m = std::min(rgba_getr(c),
                         std::min(rgba_getg(c),
                                  rgba_getb(c)));
  const int M = std::max(rgba_getr(c),
                         std::max(rgba_getg(c),
                                  rgba_getb(c)));
  return graya((M + m) / 2,
               rgba_geta(c));
}

color_t rgba_to_graya_using_luma(const color_t c)
{
  return graya(rgb_luma(rgba_getr(c),
                        rgba_getg(c),
                        rgba_getb(c)),
               rgba_geta(c));
}

} // namespace doc
