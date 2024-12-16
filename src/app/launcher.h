// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_LAUNCHER_H_INCLUDED
#define APP_LAUNCHER_H_INCLUDED
#pragma once

#include <string>

namespace app { namespace launcher {

void open_url(const std::string& url);
void open_file(const std::string& file);
void open_folder(const std::string& file);

}} // namespace app::launcher

#endif
