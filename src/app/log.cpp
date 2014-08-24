/* Aseprite
 * Copyright (C) 2001-2014  David Capello
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

namespace app {

static FILE* log_fileptr = NULL;
static LoggerModule* logger_instance = NULL;

LoggerModule::LoggerModule(bool verbose)
  : m_verbose(verbose)
{
  logger_instance = this;
}

LoggerModule::~LoggerModule()
{
  PRINTF("Logger module: shutting down (this is the last line)\n");

  if (log_fileptr) {
    fclose(log_fileptr);
    log_fileptr = NULL;
  }

  logger_instance = NULL;
}

} // namespace app

void verbose_printf(const char* format, ...)
{
  if (app::logger_instance && !app::logger_instance->isVerbose())
    return;

  if (!app::log_fileptr) {
    std::string filename;
    app::ResourceFinder rf(false);
    rf.includeUserDir("aseprite.log");
    filename = rf.defaultFilename();

    if (filename.size() > 0)
      app::log_fileptr = fopen(filename.c_str(), "w");
  }

  if (app::log_fileptr) {
    va_list ap;
    va_start(ap, format);
    vfprintf(app::log_fileptr, format, ap);
    fflush(app::log_fileptr);
    va_end(ap);
  }
}
