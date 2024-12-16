// Aseprite
// Copyright (C) 2022  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_FILE_SPLIT_FILENAME_H_INCLUDED
#define APP_FILE_SPLIT_FILENAME_H_INCLUDED
#pragma once

#include <string>

namespace app {

int split_filename(const std::string& filename, std::string& left, std::string& right, int& width);

} // namespace app

#endif
