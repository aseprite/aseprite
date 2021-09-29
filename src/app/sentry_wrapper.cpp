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
#include "base/fs.h"
#include "base/string.h"
#include "ver/info.h"

#include "sentry.h"

namespace app {

// Directory where Sentry database is saved.
std::string Sentry::m_dbdir;

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

  // We require the user consent to upload files.
  sentry_options_set_require_user_consent(options, 1);

  if (sentry_init(options) == 0)
    m_init = true;
}

Sentry::~Sentry()
{
  if (m_init)
    sentry_close();
}

// static
void Sentry::setUserID(const std::string& uuid)
{
  sentry_value_t user = sentry_value_new_object();
  sentry_value_set_by_key(user, "id", sentry_value_new_string(uuid.c_str()));
  sentry_set_user(user);
}

// static
bool Sentry::requireConsent()
{
  return (sentry_user_consent_get() != SENTRY_USER_CONSENT_GIVEN);
}

// static
bool Sentry::consentGiven()
{
  return (sentry_user_consent_get() == SENTRY_USER_CONSENT_GIVEN);
}

// static
void Sentry::giveConsent()
{
  sentry_user_consent_give();
}

// static
void Sentry::revokeConsent()
{
  sentry_user_consent_revoke();
}

// static
bool Sentry::areThereCrashesToReport()
{
  if (m_dbdir.empty())
    return false;

  for (auto f : base::list_files(base::join_path(m_dbdir, "completed"))) {
    if (base::get_file_extension(f) == "dmp")
      return true;              // At least one .dmp file in the completed/ directory
  }
  return false;
}

void Sentry::setupDirs(sentry_options_t* options)
{
  ResourceFinder rf;
  rf.includeUserDir("crashdb");
  const std::string dir = rf.getFirstOrCreateDefault();

#if SENTRY_PLATFORM_WINDOWS
  sentry_options_set_database_pathw(options, base::from_utf8(dir).c_str());
#else
  sentry_options_set_database_path(options, dir.c_str());
#endif

  m_dbdir = dir;
}

} // namespace app
