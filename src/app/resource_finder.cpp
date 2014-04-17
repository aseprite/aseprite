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

#include <allegro.h>
#include <cstdio>

#include "app/resource_finder.h"
#include "base/fs.h"
#include "base/path.h"

namespace app {

ResourceFinder::ResourceFinder()
{
  m_current = -1;
}

const std::string& ResourceFinder::filename() const
{
  // Throw an exception if we are out of bounds
  return m_paths.at(m_current);
}

bool ResourceFinder::first()
{
  m_current = 0;
  return (m_current < (int)m_paths.size());
}

bool ResourceFinder::next()
{
  ++m_current;
  return (m_current < (int)m_paths.size());
}

bool ResourceFinder::findFirst()
{
  while (next()) {
    PRINTF("Loading resource from \"%s\"...\n", filename().c_str());

    if (base::is_file(filename())) {
      PRINTF("- OK\n");
      return true;
    }
  }

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

#if defined ALLEGRO_UNIX || defined ALLEGRO_MACOSX

  // $HOME/.aseprite/filename
  sprintf(buf, ".aseprite/%s", filename);
  includeHomeDir(buf);

  // $BINDIR/data/filename
  sprintf(buf, "data/%s", filename);
  includeBinDir(buf);

  // $BINDIR/../share/aseprite/data/filename
  sprintf(buf, "../share/aseprite/data/%s", filename);
  includeBinDir(buf);

  #ifdef ALLEGRO_MACOSX
    // $BINDIR/aseprite.app/Contents/Resources/data/filename
    sprintf(buf, "aseprite.app/Contents/Resources/data/%s", filename);
    includeBinDir(buf);
  #endif

#elif defined ALLEGRO_WINDOWS || defined ALLEGRO_DOS

  // $BINDIR/data/filename
  sprintf(buf, "data/%s", filename);
  includeBinDir(buf);

#else

  // filename
  addPath(filename);

#endif
}


void ResourceFinder::includeDocsDir(const char* filename)
{
  char buf[4096];

#if defined ALLEGRO_UNIX || defined ALLEGRO_MACOSX

  // $BINDIR/docs/filename
  sprintf(buf, "docs/%s", filename);
  includeBinDir(buf);

  // $BINDIR/../share/aseprite/docs/filename
  sprintf(buf, "../share/aseprite/docs/%s", filename);
  includeBinDir(buf);

  #ifdef ALLEGRO_MACOSX
    // $BINDIR/aseprite.app/Contents/Resources/docs/filename
    sprintf(buf, "aseprite.app/Contents/Resources/docs/%s", filename);
    includeBinDir(buf);
  #endif

#elif defined ALLEGRO_WINDOWS || defined ALLEGRO_DOS

  // $BINDIR/docs/filename
  sprintf(buf, "docs/%s", filename);
  includeBinDir(buf);

#else

  // filename
  addPath(filename);

#endif
}

void ResourceFinder::includeHomeDir(const char* filename)
{
#if defined ALLEGRO_UNIX || defined ALLEGRO_MACOSX

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

#elif defined ALLEGRO_WINDOWS || defined ALLEGRO_DOS

  // $PREFIX/data/filename
  includeDataDir(filename);

#else

  // filename
  addPath(filename);

#endif
}

void ResourceFinder::includeConfFile()
{
#if defined ALLEGRO_UNIX || defined ALLEGRO_MACOSX

  // $HOME/.asepriterc
  includeHomeDir(".asepriterc");

#endif

  // $BINDIR/aseprite.ini
  includeBinDir("aseprite.ini");
}

} // namespace app
