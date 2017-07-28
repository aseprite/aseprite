// Aseprite
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/util/readable_time.h"

#include <cstdio>

namespace app {

std::string human_readable_time(const int t)
{
  char buf[32];
  if (t < 900)
    std::sprintf(buf, "%dms", t);
  else if (t < 1000*59)
    std::sprintf(buf, "%0.2fs", double(t) / 1000.0);
  else if (t < 1000*60*59)
    std::sprintf(buf, "%0.2fm", double(t) / 1000.0 / 60.0);
  else
    std::sprintf(buf, "%0.2fh", double(t) / 1000.0 / 60.0 / 60.0);
  return buf;
}

} // namespace app
