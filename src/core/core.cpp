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

#include <allegro/file.h>
#include <allegro/unicode.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <string>

#include "jinete/jbase.h"

#include "core/core.h"
#include "resource_finder.h"

/* DOS and Windows needs "log" files (because their poor stderr support) */
#if defined ALLEGRO_DOS || defined ALLEGRO_WINDOWS
#  define NEED_LOG
#endif

/* in DOS, print in stderr is like print in the video screen */
#if defined ALLEGRO_DOS
#  define NO_STDERR
#endif

/**
 * Current running mode of ASE.
 *
 * In beta releases it starts with MODE_VERBOSE value.
 */
int ase_mode = 0;

#ifdef NEED_LOG
/* log file info */
static std::string log_filename;
static FILE *log_fileptr = NULL;
#endif

LoggerModule::LoggerModule()
{
  PRINTF("Logger module: starting\n");

#ifdef NEED_LOG
  ResourceFinder rf;
  rf.findInBinDir("aseprite.log");
  log_filename = rf.first();
#endif

  PRINTF("Logger module: started\n");
}

LoggerModule::~LoggerModule()
{
  PRINTF("Logger module: shutting down (this is the last line)\n");

#ifdef NEED_LOG
  if (log_fileptr) {
    fclose(log_fileptr);
    log_fileptr = NULL;
  }
#endif
}

void verbose_printf(const char *format, ...)
{
  if (!(ase_mode & MODE_VERBOSE))
    return;

#ifdef NEED_LOG
  if (!log_fileptr)
    if (log_filename.size() > 0)
      log_fileptr = fopen(log_filename.c_str(), "w");

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

/**
 * Returns true if the application is running in interactive mode (GUI).
 *
 * Now that the application doesn't support scripting, this always returns true.
 */
bool is_interactive()
{
  return ase_mode & MODE_GUI ? true: false;
}

/**
 * Like 'strerror' but thread-safe.
 *
 * @return The same @a buf pointer.
 */
char *get_errno_string(int errnum, char *buf, int size)
{
  static const char *errors[] = {
    "No error",					/* errno = 0 */
    "Operation not permitted",			/* errno = 1 (EPERM) */
    "No such file or directory",		/* errno = 2 (ENOFILE) */
    "No such process",				/* errno = 3 (ESRCH) */
    "Interrupted function call",		/* errno = 4 (EINTR) */
    "Input/output error",			/* errno = 5 (EIO) */
    "No such device or address",		/* errno = 6 (ENXIO) */
    "Arg list too long",			/* errno = 7 (E2BIG) */
    "Exec format error",			/* errno = 8 (ENOEXEC) */
    "Bad file descriptor",			/* errno = 9 (EBADF) */
    "No child processes",			/* errno = 10 (ECHILD) */
    "Resource temporarily unavailable",		/* errno = 11 (EAGAIN) */
    "Not enough space",				/* errno = 12 (ENOMEM) */
    "Permission denied",			/* errno = 13 (EACCES) */
    "Bad address",				/* errno = 14 (EFAULT) */
    NULL,
    "Resource device",				/* errno = 16 (EBUSY) */
    "File exists",				/* errno = 17 (EEXIST) */
    "Improper link",				/* errno = 18 (EXDEV) */
    "No such device",				/* errno = 19 (ENODEV) */
    "Not a directory",				/* errno = 20 (ENOTDIR) */
    "Is a directory",				/* errno = 21 (EISDIR) */
    "Invalid argument",				/* errno = 22 (EINVAL) */
    "Too many open files in system",		/* errno = 23 (ENFILE) */
    "Too many open files",			/* errno = 24 (EMFILE) */
    "Inappropriate I/O control operation",	/* errno = 25 (ENOTTY) */
    NULL,
    "File too large",				/* errno = 27 (EFBIG) */
    "No space left on device",			/* errno = 28 (ENOSPC) */
    "Invalid seek",				/* errno = 29 (ESPIPE) */
    "Read-only file system",			/* errno = 30 (EROFS) */
    "Too many links",				/* errno = 31 (EMLINK) */
    "Broken pipe",				/* errno = 32 (EPIPE) */
    "Domain error",				/* errno = 33 (EDOM) */
    "Result too large",				/* errno = 34 (ERANGE) */
    NULL,
    "Resource deadlock avoided",		/* errno = 36 (EDEADLOCK) */
    NULL,
    "Filename too long",			/* errno = 38 (ENAMETOOLONG) */
    "No locks available",			/* errno = 39 (ENOLCK) */
    "Function not implemented",			/* errno = 40 (ENOSYS) */
    "Directory not empty",			/* errno = 41 (ENOTEMPTY) */
    "Illegal byte sequence",			/* errno = 42 (EILSEQ) */
  };

  if (errnum >= 0
      && errnum < (int)(sizeof(errors)/sizeof(char *))
      && errors[errnum] != NULL) {
    ustrncpy(buf, errors[errnum], size);
  }
  else {
    ustrncpy(buf, "Unknown error", size);
  }

  return buf;
}
