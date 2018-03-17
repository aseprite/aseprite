// Aseprite Document Library
// Copyright (c) 2001-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_ANIDIR_H_INCLUDED
#define DOC_ANIDIR_H_INCLUDED
#pragma once

#include <string>

namespace doc {

  enum class AniDir {
    FORWARD = 0,
    REVERSE = 1,
    PING_PONG = 2,
  };

  std::string convert_anidir_to_string(AniDir anidir);
  doc::AniDir convert_string_to_anidir(const std::string& s);

} // namespace doc

#endif // DOC_ANIDIR_H_INCLUDED
