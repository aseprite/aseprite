/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include <cstdio>
#include <cstring>
#include <algorithm>

#include "app.h"
#include "core/cfg.h"
#include "modules/gui.h"
#include "recent_files.h"

static RecentFiles::FilesList recent_files;
static int recent_files_limit = 16;

RecentFiles::RecentFiles()
{
  const char *filename;
  char buf[512];
  int c;

  for (c=recent_files_limit-1; c>=0; c--) {
    sprintf(buf, "Filename%02d", c);

    filename = get_config_string("RecentFiles", buf, NULL);
    if ((filename) && (*filename))
      addRecentFile(filename);
  }
}

RecentFiles::~RecentFiles()
{
  char buf[512];
  int c = 0;

  for (const_iterator it = begin(); it != end(); ++it) {
    const char* filename = it->c_str();
    sprintf(buf, "Filename%02d", c);
    set_config_string("RecentFiles", buf, filename);
    c++;
  }

  recent_files.clear();
}

RecentFiles::const_iterator RecentFiles::begin()
{
  return recent_files.begin();
}

RecentFiles::const_iterator RecentFiles::end()
{
  return recent_files.end();
}

void RecentFiles::addRecentFile(const char* filename)
{
  iterator it = std::find(recent_files.begin(), recent_files.end(), filename);

  // If the filename already exist in the list...
  if (it != recent_files.end()) {
    // Move it to the first position
    recent_files.erase(it);
    recent_files.insert(recent_files.begin(), filename);
    schedule_rebuild_recent_list();
    return;
  }

  // If the filename does not exist...

  // Does the list is full?
  if ((int)recent_files.size() == recent_files_limit) {
    // Remove the last entry
    recent_files.erase(--recent_files.end());
  }

  recent_files.insert(recent_files.begin(), filename);
  schedule_rebuild_recent_list();
}

void RecentFiles::removeRecentFile(const char* filename)
{
  iterator it = std::find(recent_files.begin(), recent_files.end(), filename);

  if (it != recent_files.end()) {
    recent_files.erase(it);

    schedule_rebuild_recent_list();
  }
}
