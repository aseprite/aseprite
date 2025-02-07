// Aseprite
// Copyright (C) 2021-2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/sentry_wrapper.h"

#include "app/resource_finder.h"
#include "base/fs.h"
#include "base/log.h"
#include "base/replace_string.h"
#include "base/string.h"
#include "ver/info.h"

#include "sentry.h"

namespace app {

// Directory where Sentry database is saved.
std::string Sentry::m_dbdir;

void Sentry::init()
{
  sentry_options_t* options = sentry_options_new();
  sentry_options_set_dsn(options, SENTRY_DSN);
  sentry_options_set_environment(options, SENTRY_ENV);

  std::string release = "aseprite@";
  release += get_app_version();

  // Remove CPU architecture from the Sentry release version (as the
  // architecture is displayed by Sentry anyway and we can group
  // events/crashes by version).
  base::replace_string(release, "-x64", "");
  base::replace_string(release, "-arm64", "");
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

  // As we don't use sentry_clear_crashed_last_run(), this will
  // return 1 if the last run (or any previous run) has crashed.
  if (sentry_get_crashed_last_run() == 1)
    return true;

  // If the last_crash file exists, we can say that there are
  // something to report (this file is created on Windows and Linux).
  if (base::is_file(base::join_path(m_dbdir, "last_crash")))
    return true;

  // At least one .dmp file in the completed/ directory means that
  // there was at least one crash in the past (this is for macOS).
  if (!base::list_files(base::join_path(m_dbdir, "completed"), base::ItemType::Files, "*.dmp")
         .empty())
    return true;

  // In case that "last_crash" doesn't exist we can check for some
  // .dmp file in the reports/ directory (it looks like the completed/
  // directory is not generated on Windows).
  if (!base::list_files(base::join_path(m_dbdir, "reports"), base::ItemType::Files, "*.dmp").empty())
    return true;

  return false;
}

// static
void Sentry::addBreadcrumb(const char* message)
{
  LOG(VERBOSE, "BC: %s\n", message);

  sentry_value_t c = sentry_value_new_breadcrumb(nullptr, message);
  sentry_add_breadcrumb(c);
}

// static
void Sentry::addBreadcrumb(const std::string& message)
{
  addBreadcrumb(message.c_str());
}

// static
void Sentry::addBreadcrumb(const std::string& message,
                           const std::map<std::string, std::string>& data)
{
  LOG(VERBOSE, "BC: %s\n", message.c_str());

  sentry_value_t c = sentry_value_new_breadcrumb(nullptr, message.c_str());
  sentry_value_t d = sentry_value_new_object();
  for (const auto& kv : data) {
    LOG(VERBOSE, " - [%s]=%s\n", kv.first.c_str(), kv.second.c_str());
    sentry_value_set_by_key(d, kv.first.c_str(), sentry_value_new_string(kv.second.c_str()));
  }
  sentry_value_set_by_key(c, "data", d);
  sentry_add_breadcrumb(c);
}

void Sentry::setupDirs(sentry_options_t* options)
{
  // The expected handler executable name is aseprite_crashpad_handler (.exe)
  const std::string handler = base::join_path(base::get_file_path(base::get_app_path()),
                                              "aseprite_crashpad_handler"
#if LAF_WINDOWS
                                              ".exe"
#endif
  );

  // The crash database will be located in the user directory as the
  // "crashdb" directory (along with "sessions", "extensions", etc.)
  ResourceFinder rf;
  rf.includeUserDir("crashdb");
  const std::string dir = rf.getFirstOrCreateDefault();

#if LAF_WINDOWS
  sentry_options_set_handler_pathw(options, base::from_utf8(handler).c_str());
  sentry_options_set_database_pathw(options, base::from_utf8(dir).c_str());
#else
  sentry_options_set_handler_path(options, handler.c_str());
  sentry_options_set_database_path(options, dir.c_str());
#endif

  m_dbdir = dir;
}

} // namespace app
