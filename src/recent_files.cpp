/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#include "config.h"

#include "recent_files.h"

#include "app.h"
#include "base/path.h"
#include "ini_file.h"

#include <cstdio>
#include <cstring>
#include <set>

RecentFiles::RecentFiles()
  : m_files(16)
  , m_paths(16)
{
  char buf[512];

  for (int c=m_files.limit()-1; c>=0; c--) {
    sprintf(buf, "Filename%02d", c);

    const char* filename = get_config_string("RecentFiles", buf, NULL);
    if (filename && *filename)
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
      base::string path = base::get_file_path(*it);

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
  app_rebuild_recent_list();
}

void RecentFiles::removeRecentFile(const char* filename)
{
  m_files.removeItem(filename);
  m_paths.removeItem(base::get_file_path(filename));
  app_rebuild_recent_list();
}
