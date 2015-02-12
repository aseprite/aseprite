// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_FILE_SPLIT_FILENAME_H_INCLUDED
#define APP_FILE_SPLIT_FILENAME_H_INCLUDED
#pragma once

#include <string>

namespace app {

  int split_filename(const char* filename, std::string& left, std::string& right, int& width);

} // namespace app

#endif
