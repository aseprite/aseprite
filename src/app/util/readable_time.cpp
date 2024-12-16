// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/util/readable_time.h"

#include "fmt/format.h"

namespace app {

std::string human_readable_time(const int t)
{
  if (t < 900)
    return fmt::format("{:d}ms", t);
  else if (t < 1000 * 59)
    return fmt::format("{:0.2f}s", double(t) / 1000.0);
  else if (t < 1000 * 60 * 59)
    return fmt::format("{:0.2f}m", double(t) / 1000.0 / 60.0);
  else
    return fmt::format("{:0.2f}h", double(t) / 1000.0 / 60.0 / 60.0);
}

} // namespace app
