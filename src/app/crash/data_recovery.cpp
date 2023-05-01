// Aseprite
// Copyright (C) 2019-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/crash/data_recovery.h"

#include "app/crash/backup_observer.h"
#include "app/crash/log.h"
#include "app/crash/session.h"
#include "app/pref/preferences.h"
#include "app/resource_finder.h"
#include "base/fs.h"
#include "base/time.h"
#include "ui/system.h"

#include <algorithm>
#include <chrono>
#include <thread>

namespace app {
namespace crash {

// Flag used to avoid calling SessionsListIsReady() signal after
// DataRecovery() instance is deleted.
static bool g_stillAliveFlag = false;

DataRecovery::DataRecovery(Context* ctx)
  : m_inProgress(nullptr)
  , m_backup(nullptr)
  , m_searching(false)
{
  auto& pref = Preferences::instance();
  m_config.dataRecoveryPeriod = pref.general.dataRecoveryPeriod();
  if (pref.general.keepEditedSpriteData())
    m_config.keepEditedSpriteDataFor = pref.general.keepEditedSpriteDataFor();
  else
    m_config.keepEditedSpriteDataFor = 0;

  ResourceFinder rf;
  rf.includeUserDir(base::join_path("sessions", ".").c_str());
  m_sessionsDir = rf.getFirstOrCreateDefault();

  // Create a new session
  base::pid pid = base::get_current_process_id();
  std::string newSessionDir;

  do {
    base::Time time = base::current_time();

    char buf[1024];
    sprintf(buf, "%04d%02d%02d-%02d%02d%02d-%d",
      time.year, time.month, time.day,
      time.hour, time.minute, time.second, pid);

    newSessionDir = base::join_path(m_sessionsDir, buf);

    if (!base::is_directory(newSessionDir))
      base::make_directory(newSessionDir);
    else {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      newSessionDir.clear();
    }
  } while (newSessionDir.empty());

  m_inProgress.reset(new Session(&m_config, newSessionDir));
  m_inProgress->create(pid);
  RECO_TRACE("RECO: Session in progress '%s'\n", newSessionDir.c_str());

  m_backup = new BackupObserver(&m_config, m_inProgress.get(), ctx);

  g_stillAliveFlag = true;
}

DataRecovery::~DataRecovery()
{
  g_stillAliveFlag = false;
  m_thread.join();

  m_backup->stop();
  delete m_backup;

  // We just close the session on progress.  The session is not
  // deleted just in case that the user want to recover some files
  // from this session in the future.
  if (m_inProgress)
    m_inProgress->close();

  m_inProgress.reset();
}

void DataRecovery::launchSearch()
{
  if (m_searching)
    return;

  // Search current sessions in a background thread
  if (m_thread.joinable())
    m_thread.join();

  ASSERT(!m_searching);
  m_searching = true;

  m_thread = std::thread(
    [this]{
      searchForSessions();
      m_searching = false;
    });
}

bool DataRecovery::hasRecoverySessions() const
{
  std::unique_lock<std::mutex> lock(m_sessionsMutex);

  for (const auto& session : m_sessions)
    if (session->isCrashedSession())
      return true;
  return false;
}

DataRecovery::Sessions DataRecovery::sessions()
{
  Sessions copy;
  {
    std::unique_lock<std::mutex> lock(m_sessionsMutex);
    copy = m_sessions;
  }
  return copy;
}

void DataRecovery::searchForSessions()
{
  Sessions sessions;

  // Existent sessions
  RECO_TRACE("RECO: Listing sessions from '%s'\n", m_sessionsDir.c_str());
  for (auto& itemname : base::list_files(m_sessionsDir)) {
    std::string itempath = base::join_path(m_sessionsDir, itemname);
    if (base::is_directory(itempath)) {
      RECO_TRACE("RECO: Session '%s'\n", itempath.c_str());

      SessionPtr session(new Session(&m_config, itempath));
      if (!session->isRunning()) {
        if ((session->isEmpty()) ||
            (!session->isCrashedSession() && session->isOldSession())) {
          RECO_TRACE("RECO: - to be deleted (%s)\n",
                     session->isEmpty() ? "is empty":
                     (session->isOldSession() ? "is old":
                                                "unknown reason"));
          session->removeFromDisk();
        }
        else {
          RECO_TRACE("RECO:  - to be loaded\n");
          sessions.push_back(session);
        }
      }
      else
        RECO_TRACE("is running\n");
    }
  }

  // Sort sessions from the most recent one to the oldest one
  std::sort(sessions.begin(), sessions.end(),
            [](const SessionPtr& a, const SessionPtr& b) {
              return a->name() > b->name();
            });

  // Assign m_sessions=sessions
  {
    std::unique_lock<std::mutex> lock(m_sessionsMutex);
    std::swap(m_sessions, sessions);
  }

  ui::execute_from_ui_thread(
    [this]{
      if (g_stillAliveFlag)
        SessionsListIsReady();
    });
}

} // namespace crash
} // namespace app
