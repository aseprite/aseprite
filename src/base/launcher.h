// Aseprite Base Library
// Copyright (c) 2001-2013, 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_LAUNCHER_H_INCLUDED
#define BASE_LAUNCHER_H_INCLUDED
#pragma once

#include <string>

namespace base {
namespace launcher {

bool open_url(const std::string& url);
bool open_file(const std::string& file);
bool open_folder(const std::string& file);

} // namespace launcher
} // namespace base

#endif
