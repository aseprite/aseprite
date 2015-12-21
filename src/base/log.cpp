// Aseprite Base Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/log.h"

#include "base/file_handle.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>

static FILE* log_fileptr = nullptr;
static std::string log_filename;

void base_set_log_filename(const char* filename)
{
  if (log_fileptr) {
    fclose(log_fileptr);
    log_fileptr = nullptr;
  }
  log_filename = filename;
}

void base_log(const char* format, ...)
{
  if (!log_fileptr) {
    if (log_filename.empty())
      return;

    log_fileptr = base::open_file_raw(log_filename, "w");
  }

  if (log_fileptr) {
    va_list ap;
    va_start(ap, format);

    vfprintf(log_fileptr, format, ap);
    fflush(log_fileptr);

#ifdef _DEBUG
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    fflush(stderr);
#endif

    va_end(ap);
  }
}
