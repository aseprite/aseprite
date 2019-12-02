// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_RESOURCE_FINDER_H_INCLUDED
#define APP_RESOURCE_FINDER_H_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "base/paths.h"

#include <string>

namespace app {

  // Helper class to find configuration files in different directories
  // in a priority order (e.g. first in the $HOME directory, then in
  // data/ directory, etc.).
  class ResourceFinder {
  public:
    ResourceFinder(bool log = true);

    // Returns the current possible path. You cannot call this
    // function if you haven't call first() or next() before.
    const std::string& filename() const;
    const std::string& defaultFilename() const;

    // Goes to next possible path.
    bool next();

    // Iterates over all possible paths and returns true if the file
    // is exists. Returns the first existent file.
    bool findFirst();

    // These functions add possible full paths to find files.
    void addPath(const std::string& path);
    void includeBinDir(const char* filename);
    void includeDataDir(const char* filename);
    void includeHomeDir(const char* filename);

    // Tries to add the given filename in these locations:
    // For Windows:
    // - If ASEPRITE_USER_FOLDER environment variable is defined, it
    //   should point the folder where the "user dir" is (it's useful
    //   for testing purposes to test with an empty preferences
    //   folder)
    // - If the app is running in portable mode, the filename
    //   will be in the same location as the .exe file.
    // - If the app is installed, the filename will be inside
    //   %AppData% location
    // For Unix-like platforms:
    // - The filename will be in $HOME/.config/aseprite/
    void includeUserDir(const char* filename);

    void includeDesktopDir(const char* filename);

    // Returns the first file found or creates the whole directory
    // structure to create the file in its default location.
    std::string getFirstOrCreateDefault();

  private:
    bool m_log;
    base::paths m_paths;
    int m_current;
    std::string m_default;

    DISABLE_COPYING(ResourceFinder);
  };

} // namespace app

#endif
