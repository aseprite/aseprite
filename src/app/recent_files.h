// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_RECENT_FILES_H_INCLUDED
#define APP_RECENT_FILES_H_INCLUDED
#pragma once

#include "base/paths.h"
#include "obs/signal.h"

#include <string>

namespace app {

  class RecentFiles {
    enum { kPinnedFiles,
           kRecentFiles,
           kPinnedFolders,
           kRecentFolders,
           kCollections };
  public:
    const base::paths& pinnedFiles() { return m_paths[kPinnedFiles]; }
    const base::paths& recentFiles() { return m_paths[kRecentFiles]; }
    const base::paths& pinnedFolders() { return m_paths[kPinnedFolders]; }
    const base::paths& recentFolders() { return m_paths[kRecentFolders]; }

    RecentFiles(const int limit);
    ~RecentFiles();

    void addRecentFile(const std::string& filename);
    void removeRecentFile(const std::string& filename);
    void removeRecentFolder(const std::string& dir);
    void setLimit(const int newLimit);
    void clear();

    void setFiles(const base::paths& pinnedFiles,
                  const base::paths& recentFiles);
    void setFolders(const base::paths& pinnedFolders,
                    const base::paths& recentFolders);

    obs::signal<void()> Changed;

  private:
    std::string normalizePath(const std::string& filename);
    void addItem(base::paths& list, const std::string& filename);
    void removeItem(base::paths& list, const std::string& filename);
    void load();
    void save();

    base::paths m_paths[kCollections];
    int m_limit;
  };

} // namespace app

#endif
