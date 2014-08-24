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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/resource_finder.h"

#include "app/app.h"
#include "base/fs.h"
#include "base/path.h"
#include "base/string.h"

#include <cstdio>

namespace app {

ResourceFinder::ResourceFinder(bool log)
  : m_log(log)
{
  m_current = -1;
}

const std::string& ResourceFinder::filename() const
{
  // Throw an exception if we are out of bounds
  return m_paths.at(m_current);
}

const std::string& ResourceFinder::defaultFilename() const
{
  if (m_default.empty()) {
    // The first path is the default one if nobody specified it.
    if (!m_paths.empty())
      return m_paths[0];
  }
  return m_default;
}

bool ResourceFinder::next()
{
  ++m_current;
  return (m_current < (int)m_paths.size());
}

bool ResourceFinder::findFirst()
{
  while (next()) {
    if (m_log)
      PRINTF("Loading resource from \"%s\"...\n", filename().c_str());

    if (base::is_file(filename())) {
      if (m_log)
        PRINTF("- OK\n");

      return true;
    }
  }

  if (m_log)
    PRINTF("- Resource not found.\n");

  return false;
}

void ResourceFinder::addPath(const std::string& path)
{
  m_paths.push_back(path);
}

void ResourceFinder::includeBinDir(const char* filename)
{
  addPath(base::join_path(base::get_file_path(base::get_app_path()), filename));
}

void ResourceFinder::includeDataDir(const char* filename)
{
  char buf[4096];

#ifdef WIN32

  // $BINDIR/data/filename
  sprintf(buf, "data/%s", filename);
  includeBinDir(buf);

#else

  // $HOME/.config/aseprite/filename
  sprintf(buf, ".config/aseprite/data/%s", filename);
  includeHomeDir(buf);

  // $BINDIR/data/filename
  sprintf(buf, "data/%s", filename);
  includeBinDir(buf);

  #ifdef __APPLE__
    // $BINDIR/../Resources/data/filename (inside a bundle)
    sprintf(buf, "../Resources/data/%s", filename);
    includeBinDir(buf);
  #else
    // $BINDIR/../share/aseprite/data/filename (installed in /usr/ or /usr/local/)
    sprintf(buf, "../share/aseprite/data/%s", filename);
    includeBinDir(buf);
  #endif

#endif
}

void ResourceFinder::includeHomeDir(const char* filename)
{
#ifdef WIN32

  // %AppData%/Aseprite/filename
  wchar_t* env = _wgetenv(L"AppData");
  if (env) {
    std::string path = base::join_path(base::to_utf8(env), "Aseprite");
    path = base::join_path(path, filename);
    addPath(path);
    m_default = path;
  }

#else

  char* env = getenv("HOME");
  char buf[4096];

  if ((env) && (*env)) {
    // $HOME/filename
    sprintf(buf, "%s/%s", env, filename);
    addPath(buf);
  }
  else {
    PRINTF("You don't have set $HOME variable\n");
    addPath(filename);
  }

#endif
}

void ResourceFinder::includeUserDir(const char* filename)
{
#ifdef WIN32

  if (App::instance()->isPortable()) {
    // $BINDIR/filename
    includeBinDir(filename);
  }
  else {
    // %AppData%/Aseprite/filename
    includeHomeDir(filename);
  }

#else

  // $HOME/.config/aseprite/filename
  includeHomeDir((std::string(".config/aseprite/") + filename).c_str());

#endif
}

std::string ResourceFinder::getFirstOrCreateDefault()
{
  std::string fn;

  // Search from first to last path
  if (findFirst())
    fn = filename();

  // If the file wasn't found, we will create the directories for the
  // default file name.
  if (fn.empty()) {
    fn = defaultFilename();

    std::string dir = base::get_file_path(fn);
    if (!base::is_directory(dir))
      base::make_all_directories(dir);
  }

  return fn;
}

} // namespace app
