// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/recent_files.h"

#include "app/app_menus.h"
#include "app/ini_file.h"
#include "base/fs.h"
#include "base/path.h"

#include <cstdio>
#include <cstring>
#include <set>

namespace app {

RecentFiles::RecentFiles()
  : m_files(16)
  , m_paths(16)
{
  char buf[512];

  for (int c=m_files.limit()-1; c>=0; c--) {
    sprintf(buf, "Filename%02d", c);

    const char* filename = get_config_string("RecentFiles", buf, NULL);
    if (filename && *filename && base::is_file(filename))
      m_files.addItem(filename);
  }

  for (int c=m_paths.limit()-1; c>=0; c--) {
    sprintf(buf, "Path%02d", c);

    const char* path = get_config_string("RecentPaths", buf, NULL);
    if (path && *path)
      m_paths.addItem(path);
  }

  // Create recent list of paths from filenames (for backward
  // compatibility with previous versions of ASEPRITE).
  if (m_paths.empty()) {
    std::set<std::string> included;

    // For each recent file...
    const_iterator it = files_begin();
    const_iterator end = files_end();
    for (; it != end; ++it) {
      std::string path = base::get_file_path(*it);

      // Check if the path was not already included in the list
      if (included.find(path) == included.end()) {
        included.insert(path);
        m_paths.addItem(path);
      }
    }
  }
}

RecentFiles::~RecentFiles()
{
  char buf[512];

  int c = 0;
  for (const_iterator it = files_begin(); it != files_end(); ++it) {
    const char* filename = it->c_str();
    sprintf(buf, "Filename%02d", c);
    set_config_string("RecentFiles", buf, filename);
    c++;
  }

  c = 0;
  for (const_iterator it = paths_begin(); it != paths_end(); ++it) {
    const char* path = it->c_str();
    sprintf(buf, "Path%02d", c);
    set_config_string("RecentPaths", buf, path);
    c++;
  }
}

void RecentFiles::addRecentFile(const char* filename)
{
  m_files.addItem(filename);
  m_paths.addItem(base::get_file_path(filename));

  AppMenus::instance()->rebuildRecentList();
}

void RecentFiles::removeRecentFile(const char* filename)
{
  m_files.removeItem(filename);
  m_paths.removeItem(base::get_file_path(filename));

  AppMenus::instance()->rebuildRecentList();
}

} // namespace app
