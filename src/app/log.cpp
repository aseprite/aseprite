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

#include "app/log.h"

#include "app/app.h"
#include "app/resource_finder.h"
#include "ui/base.h"

#include <allegro/file.h>
#include <allegro/unicode.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <string>

// DOS and Windows needs "log" files (because their poor stderr support)
#if defined ALLEGRO_DOS || defined ALLEGRO_WINDOWS
#  define NEED_LOG
#endif

// in DOS, print in stderr is like print in the video screen
#if defined ALLEGRO_DOS
#  define NO_STDERR
#endif

namespace app {

#ifdef NEED_LOG                 // log file info
static FILE* log_fileptr = NULL;
#endif

static LoggerModule* logger_instance = NULL;

LoggerModule::LoggerModule(bool verbose)
  : m_verbose(verbose)
{
  logger_instance = this;
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

  logger_instance = NULL;
}

} // namespace app

void verbose_printf(const char* format, ...)
{
  using namespace app;

  if (!logger_instance) {
    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    fflush(stderr);
    va_end(ap);
    return;
  }

  if (!logger_instance->isVerbose())
    return;

#ifdef NEED_LOG
  if (!log_fileptr) {
    std::string filename;
    ResourceFinder rf;
    rf.includeBinDir("aseprite.log");
    if (rf.first())
      filename = rf.filename();

    if (filename.size() > 0)
      log_fileptr = fopen(filename.c_str(), "w");
  }

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
