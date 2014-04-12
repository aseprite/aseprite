/* Aseprite
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef APP_RESOURCE_FINDER_H_INCLUDED
#define APP_RESOURCE_FINDER_H_INCLUDED
#pragma once

#include <string>
#include <vector>

namespace app {

  // Helper class to find configuration files in different directories
  // in a priority order (e.g. first in the $HOME directory, then in
  // data/ directory, etc.).
  class ResourceFinder {
  public:
    ResourceFinder();

    // Returns the current possible path. You cannot call this
    // function if you haven't call first() or next() before.
    const std::string& filename() const;

    // Goes to the first option in the list of possible paths.
    // Returns true if there is (at least) one option available
    // (m_paths.size() != 0).
    bool first();

    // Goes to next possible path.
    bool next();

    // Iterates over all possible paths and returns true if the file
    // is exists. Returns the first existent file.
    bool findFirst();

    // These functions add possible full paths to find files.
    void addPath(const std::string& path);
    void includeBinDir(const char* filename);
    void includeDataDir(const char* filename);
    void includeDocsDir(const char* filename);
    void includeHomeDir(const char* filename);
    void includeConfFile();

  private:
    // Disable copy
    ResourceFinder(const ResourceFinder&);
    ResourceFinder& operator==(const ResourceFinder&);

    // Members
    std::vector<std::string> m_paths;
    int m_current;
  };

} // namespace app

#endif
