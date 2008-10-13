/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2008  David A. Capello
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

#include <allegro/file.h>
#include <allegro/system.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jinete/jbase.h"

#include "core/core.h"
#include "core/dirs.h"

DIRS *dirs_new()
{
  DIRS* dirs;

  dirs = (DIRS*)jmalloc(sizeof(DIRS));
  if (!dirs)
    return NULL;

  dirs->path = NULL;
  dirs->next = NULL;

  return dirs;
}

void dirs_free(DIRS *dirs)
{
  DIRS *iter, *next;

  for (iter=dirs; iter; iter=next) {
    next = iter->next;
    if (iter->path)
      jfree(iter->path);

    jfree(iter);
  }
}

void dirs_add_path(DIRS *dirs, const char *path)
{
  if (!dirs->path) {
    dirs->path = jstrdup(path);
  }
  else {
    DIRS *newone, *lastone;

    for (lastone=dirs; lastone->next; lastone=lastone->next)
      ;

    newone = dirs_new();
    if (!newone)
      return;

    newone->path = jstrdup(path);
    if (!newone->path) {
      jfree(newone);
      return;
    }

    lastone->next = newone;
  }
}

void dirs_cat_dirs(DIRS *dirs, DIRS *more)
{
  DIRS *lastone, *origmore = more;

  if (!dirs->path) {
    dirs->path = more->path;
    more = more->next;

    jfree(origmore);
  }

  for (lastone=dirs; lastone->next; lastone=lastone->next)
    ;

  lastone->next = more;
}

DIRS *filename_in_bindir(const char *filename)
{
  char buf[1024], path[1024];
  DIRS *dirs;

  get_executable_name(path, sizeof(path));
  replace_filename(buf, path, filename, sizeof(buf));

  dirs = dirs_new();
  dirs_add_path(dirs, buf);

  return dirs;
}

DIRS *filename_in_datadir(const char *filename)
{
  DIRS *dirs = dirs_new();
  char buf[1024];

#if defined ALLEGRO_UNIX

  /* $HOME/.ase/filename */
  sprintf(buf, ".ase/%s", filename);
  dirs_cat_dirs(dirs, filename_in_homedir(buf));

  /* $BINDIR/data/filename */
  sprintf(buf, "data/%s", filename);
  dirs_cat_dirs(dirs, filename_in_bindir(buf));
  
  #ifdef DEFAULT_PREFIX
    /* $PREFIX/ase/filename */
    sprintf(buf, "%s/share/ase/%s", DEFAULT_PREFIX, filename);
    dirs_add_path(dirs, buf);
  #endif

#elif defined ALLEGRO_WINDOWS || defined ALLEGRO_DOS

  /* $BINDIR/data/filename */
  sprintf(buf, "data/%s", filename);
  dirs_cat_dirs(dirs, filename_in_bindir(buf));

#else

  /* filename */
  dirs_add_path(dirs, filename);

#endif

  return dirs;
}

DIRS *filename_in_homedir(const char *filename)
{
  DIRS *dirs = dirs_new();

#if defined ALLEGRO_UNIX

  char *env = getenv("HOME");
  char buf[1024];

  if ((env) && (*env)) {
    /* $HOME/filename */
    sprintf(buf, "%s/%s", env, filename);
    dirs_add_path(dirs, buf);
  }
  else {
    PRINTF("You don't have set $HOME variable\n");
    dirs_add_path(dirs, filename);
  }

#elif defined ALLEGRO_WINDOWS || defined ALLEGRO_DOS

  /* $PREFIX/data/filename */
  dirs_cat_dirs(dirs, filename_in_datadir(filename));

#else

  /* filename */
  dirs_add_path(dirs, filename);

#endif

  return dirs;
}

DIRS *cfg_filename_dirs()
{
  DIRS *dirs = dirs_new();

#if defined ALLEGRO_UNIX

  /* $HOME/.aserc-VERSION */
  dirs_cat_dirs(dirs, filename_in_homedir(".aserc-" VERSION));

#endif

  /* $BINDIR/ase-VERSION.cfg */
  dirs_cat_dirs(dirs, filename_in_bindir("ase-" VERSION ".cfg"));

  return dirs;
}
