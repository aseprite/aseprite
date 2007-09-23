/* ase -- allegro-sprite-editor: the ultimate sprites factory
 * Copyright (C) 2001-2005, 2007  David A. Capello
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

#ifndef USE_PRECOMPILED_HEADER

#include <allegro/file.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "jinete/base.h"

#include "core/core.h"
#include "core/dirs.h"

#endif

/* DOS and Windows needs "log" files (because their poor stderr support) */
#if defined ALLEGRO_DOS || defined ALLEGRO_WINDOWS
#  define NEED_LOG
#endif

/* in DOS, print in stderr is like print in the video screen */
#if defined ALLEGRO_DOS
#  define NO_STDERR
#endif

/* ASE mode */
int ase_mode = 0;

#ifdef NEED_LOG
/* log file info */
static char *log_filename = NULL;
static FILE *log_fileptr = NULL;
#endif

int core_init(void)
{
#ifdef NEED_LOG
  char buf[512];
  int count = 0;
  DIRS *dirs;

  for (;;) {
    sprintf(buf, "log%04d.txt", count++);
    dirs = filename_in_bindir(buf);
    if (!exists(dirs->path))
      break;
    else
      dirs_free(dirs);
  }

  log_filename = jstrdup(dirs->path);
  dirs_free(dirs);
#endif

  return 0;
}

void core_exit(void)
{
#ifdef NEED_LOG
  if (log_fileptr) {
    fclose (log_fileptr);
    log_fileptr = NULL;
  }

  if (log_filename) {
    free (log_filename);
    log_filename = NULL;
  }
#endif
}

void verbose_printf(const char *format, ...)
{
  if (!(ase_mode & MODE_VERBOSE))
    return;

#ifdef NEED_LOG
  if (!log_fileptr)
    if (log_filename)
      log_fileptr = fopen(log_filename, "w");

  if (log_fileptr)
#endif
    {
      va_list ap;
      va_start(ap, format);

#ifndef NO_STDERR
      vfprintf(stderr, format, ap);
      fflush(stderr);
#endif

#ifdef NEED_LOG
      vfprintf(log_fileptr, format, ap);
      fflush(log_fileptr);
#endif

      va_end(ap);
    }
}

bool is_interactive(void)
{
  return ase_mode & MODE_GUI;
}
