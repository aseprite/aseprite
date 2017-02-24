// Aseprite
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_FONT_PATH_H_INCLUDED
#define APP_FONT_PATH_H_INCLUDED
#pragma once

#include <string>
#include <vector>

namespace app {

  void get_font_dirs(std::vector<std::string>& fontDirs);
  std::string find_font(const std::string& filename);

} // namespace app

#endif
