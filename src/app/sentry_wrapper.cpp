// Aseprite
// Copyright (C) 2021  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/sentry_wrapper.h"

#include "app/resource_finder.h"
#include "base/string.h"
#include "ver/info.h"

#include "sentry.h"

namespace app {

void Sentry::init()
{
  sentry_options_t* options = sentry_options_new();
  sentry_options_set_dsn(options, SENTRY_DNS);

  std::string release = "aseprite@";
  release += get_app_version();
  sentry_options_set_release(options, release.c_str());

#if _DEBUG
  sentry_options_set_debug(options, 1);
#endif

  setupDirs(options);

  if (sentry_init(options) == 0)
    m_init = true;
}

Sentry::~Sentry()
{
  if (m_init)
    sentry_close();
}

void Sentry::setupDirs(sentry_options_t* options)
{
  ResourceFinder rf;
  rf.includeUserDir("crashdb");
  std::string dir = rf.getFirstOrCreateDefault();

#if SENTRY_PLATFORM_WINDOWS
  sentry_options_set_database_pathw(options, base::from_utf8(dir).c_str());
#else
  sentry_options_set_database_path(options, dir.c_str());
#endif
}

} // namespace app
