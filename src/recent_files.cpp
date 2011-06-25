/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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
#include "recent_files.h"

RecentFiles::RecentFiles()
  : m_limit(16)
{
  const char* filename;
  char buf[512];
  int c;

  for (c=m_limit-1; c>=0; c--) {
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

  m_files.clear();
}

RecentFiles::const_iterator RecentFiles::begin()
{
  return m_files.begin();
}

RecentFiles::const_iterator RecentFiles::end()
{
  return m_files.end();
}

void RecentFiles::addRecentFile(const char* filename)
{
  iterator it = std::find(m_files.begin(), m_files.end(), filename);

  // If the filename already exist in the list...
  if (it != m_files.end()) {
    // Move it to the first position
    m_files.erase(it);
    m_files.insert(m_files.begin(), filename);

    app_rebuild_recent_list();
    return;
  }

  // If the filename does not exist...

  // Does the list is full?
  if ((int)m_files.size() == m_limit) {
    // Remove the last entry
    m_files.erase(--m_files.end());
  }

  m_files.insert(m_files.begin(), filename);
  app_rebuild_recent_list();
}

void RecentFiles::removeRecentFile(const char* filename)
{
  iterator it = std::find(m_files.begin(), m_files.end(), filename);

  if (it != m_files.end()) {
    m_files.erase(it);
    app_rebuild_recent_list();
  }
}
