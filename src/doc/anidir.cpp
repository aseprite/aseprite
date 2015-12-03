// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/anidir.h"

namespace doc {

std::string convert_to_string(AniDir anidir)
{
  switch (anidir) {
    case AniDir::FORWARD: return "forward";
    case AniDir::REVERSE: return "reverse";
    case AniDir::PING_PONG: return "pingpong";
  }
  return "";
}

} // namespace doc
