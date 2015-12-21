// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/log.h"

#include "app/app.h"
#include "app/resource_finder.h"
#include "base/log.h"

namespace app {

static LoggerModule* logger_instance = NULL;

LoggerModule::LoggerModule(bool verbose)
  : m_verbose(verbose)
{
  logger_instance = this;

  if (verbose) {
    app::ResourceFinder rf(false);
    rf.includeUserDir("aseprite.log");
    auto filename = rf.defaultFilename();
    base_set_log_filename(filename.c_str());
  }
}

LoggerModule::~LoggerModule()
{
  LOG("Logger module: shutting down (this is the last line)\n");

  // Close log file
  base_set_log_filename("");
  logger_instance = nullptr;
}

} // namespace app
