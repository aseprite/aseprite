// Aseprite
// Copyright (C) 2017-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_FONT_PATH_H_INCLUDED
#define APP_FONT_PATH_H_INCLUDED
#pragma once

#include "base/paths.h"

#include <string>

namespace app {

void get_font_dirs(base::paths& fontDirs);
std::string find_font(const std::string& firstDir, const std::string& filename);

} // namespace app

#endif
