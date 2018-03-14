// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_RECENT_FILES_H_INCLUDED
#define APP_RECENT_FILES_H_INCLUDED
#pragma once

#include "base/recent_items.h"
#include "obs/signal.h"

#include <string>

namespace app {

  class RecentFiles {
  public:
    typedef base::RecentItems<std::string> List;
    typedef List::iterator iterator;
    typedef List::const_iterator const_iterator;

    // Iterate through recent files.
    const_iterator files_begin() { return m_files.begin(); }
    const_iterator files_end() { return m_files.end(); }

    // Iterate through recent paths.
    const_iterator paths_begin() { return m_paths.begin(); }
    const_iterator paths_end() { return m_paths.end(); }

    RecentFiles(const int limit);
    ~RecentFiles();

    void addRecentFile(const std::string& filename);
    void removeRecentFile(const std::string& filename);
    void removeRecentFolder(const std::string& dir);
    void setLimit(const int n);
    void clear();

    obs::signal<void()> Changed;

  private:
    std::string normalizePath(const std::string& filename);

    List m_files;
    List m_paths;
  };

} // namespace app

#endif
