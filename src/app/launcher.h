// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_LAUNCHER_H_INCLUDED
#define APP_LAUNCHER_H_INCLUDED
#pragma once

#include <string>

namespace app {
  namespace launcher {

    void open_url(const std::string& url);
    void open_file(const std::string& file);
    void open_folder(const std::string& file);

  } // namespace launcher
} // namespace app

#endif
