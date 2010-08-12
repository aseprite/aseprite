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

#include <allegro.h>
#include <cstdio>

#include "resource_finder.h"

ResourceFinder::ResourceFinder()
{
  m_current = 0;
}

const char* ResourceFinder::first()
{
  if (!m_paths.empty())
    return m_paths[0].c_str();
  else
    return NULL;
}

const char* ResourceFinder::next()
{
  if (m_current == (int)m_paths.size())
    return NULL;

  return m_paths[m_current++].c_str();
}

void ResourceFinder::addPath(std::string path)
{
  m_paths.push_back(path);
}

void ResourceFinder::findInBinDir(const char* filename)
{
  char buf[1024], path[1024];

  get_executable_name(path, sizeof(path));
  replace_filename(buf, path, filename, sizeof(buf));

  addPath(buf);
}

void ResourceFinder::findInDataDir(const char* filename)
{
  char buf[1024];

#if defined ALLEGRO_UNIX || defined ALLEGRO_MACOSX

  // $HOME/.ase/filename
  sprintf(buf, ".ase/%s", filename);
  findInHomeDir(buf);

  // $BINDIR/data/filename
  sprintf(buf, "data/%s", filename);
  findInBinDir(buf);
  
  #ifdef DEFAULT_PREFIX
    // $PREFIX/ase/filename
    sprintf(buf, "%s/share/ase/%s", DEFAULT_PREFIX, filename);
    addPath(buf);
  #endif

  #ifdef ALLEGRO_MACOSX
    // $BINDIR/aseprite.app/Contents/Resources/data/filename
    sprintf(buf, "aseprite.app/Contents/Resources/data/%s", filename);
    findInBinDir(buf);
  #endif

#elif defined ALLEGRO_WINDOWS || defined ALLEGRO_DOS

  // $BINDIR/data/filename
  sprintf(buf, "data/%s", filename);
  findInBinDir(buf);

#else

  // filename
  addPath(filename);

#endif
}


void ResourceFinder::findInDocsDir(const char* filename)
{
  char buf[1024];

#if defined ALLEGRO_UNIX || defined ALLEGRO_MACOSX

  // $BINDIR/docs/filename
  sprintf(buf, "docs/%s", filename);
  findInBinDir(buf);
  
  #ifdef DEFAULT_PREFIX
    // $PREFIX/ase/docs/filename
    sprintf(buf, "%s/share/ase/docs/%s", DEFAULT_PREFIX, filename);
    addPath(buf);
  #endif

  #ifdef ALLEGRO_MACOSX
    // $BINDIR/aseprite.app/Contents/Resources/docs/filename
    sprintf(buf, "aseprite.app/Contents/Resources/docs/%s", filename);
    findInBinDir(buf);
  #endif

#elif defined ALLEGRO_WINDOWS || defined ALLEGRO_DOS

  // $BINDIR/docs/filename
  sprintf(buf, "docs/%s", filename);
  findInBinDir(buf);

#else

  // filename
  addPath(filename);

#endif
}

void ResourceFinder::findInHomeDir(const char* filename)
{
#if defined ALLEGRO_UNIX || defined ALLEGRO_MACOSX

  char *env = getenv("HOME");
  char buf[1024];

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
  findInDataDir(filename);

#else

  // filename
  addPath(filename);

#endif
}

void ResourceFinder::findConfigurationFile()
{
#if defined ALLEGRO_UNIX || defined ALLEGRO_MACOSX

  // $HOME/.asepriterc
  findInHomeDir(".asepriterc");

#endif

  // $BINDIR/aseprite.ini
  findInBinDir("aseprite.ini");
}
