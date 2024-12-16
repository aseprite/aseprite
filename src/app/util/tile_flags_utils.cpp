// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/util/tile_flags_utils.h"

namespace app {

void build_tile_flags_string(const doc::tile_flags tf, std::string& result)
{
  if (tf & doc::tile_f_xflip)
    result += "X";
  if (tf & doc::tile_f_yflip)
    result += "Y";
  if (tf & doc::tile_f_dflip)
    result += "D";
}

} // namespace app
