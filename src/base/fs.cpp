// Aseprite Base Library
// Copyright (c) 2001-2013, 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/fs.h"
#include "base/split_string.h"

#ifdef _WIN32
  #include "base/fs_win32.h"
#else
  #include "base/fs_unix.h"
#endif

namespace base {

void make_all_directories(const std::string& path)
{
  std::vector<std::string> parts;
  split_string(path, parts, "/\\");

  std::string intermediate;
  for (const std::string& component : parts) {
    if (component.empty()) {
      if (intermediate.empty())
        intermediate += "/";
      continue;
    }

    intermediate = join_path(intermediate, component);

    if (is_file(intermediate))
      throw std::runtime_error("Error creating directory (a component is a file name)");
    else if (!is_directory(intermediate))
      make_directory(intermediate);
  }
}

} // namespace base
