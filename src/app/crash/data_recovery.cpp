// Aseprite
// Copyright (C) 2019-present  Igara Studio S.A.
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
#include "base/thread.h"
#include "base/time.h"
#include "fmt/format.h"
#include "ui/system.h"

#include <algorithm>
#include <chrono>
#include <thread>

namespace app { namespace crash {

// Flag used to avoid calling SessionsListIsReady() signal after
// DataRecovery() instance is deleted.
static bool g_stillAliveFlag = false;

DataRecovery::DataRecovery(Context* ctx)
  : m_inProgress(nullptr)
  , m_backup(nullptr)
  , m_searching(false)
{
  updateConfig();

  ResourceFinder rf;
  rf.includeUserDir(base::join_path("sessions", ".").c_str());
  m_sessionsDir = rf.getFirstOrCreateDefault();

  // Create a new session
  base::pid pid = base::get_current_process_id();
  std::string newSessionDir;

  do {
    base::Time time = base::current_time();

    std::string buf = fmt::format("{:04}{:02}{:02}-{:02}{:02}{:02}-{}",
                                  time.year,
                                  time.month,
                                  time.day,
                                  time.hour,
                                  time.minute,
                                  time.second,
                                  pid);

    newSessionDir = base::join_path(m_sessionsDir, buf);

    if (!base::is_directory(newSessionDir))
      base::make_directory(newSessionDir);
    else {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      newSessionDir.clear();
    }
  } while (newSessionDir.empty());

  m_inProgress.reset(new Session(&m_config, newSessionDir, true));
  m_inProgress->create(pid);
  RECO_TRACE("RECO: Session in progress '%s'\n", newSessionDir.c_str());

  m_backup = std::make_unique<BackupObserver>(&m_config, m_inProgress.get(), ctx);

  g_stillAliveFlag = true;
}

DataRecovery::~DataRecovery()
{
  g_stillAliveFlag = false;
  if (m_thread.joinable())
    m_thread.join();

  m_backup->stop();
  m_backup.reset();

  // We just close the session on progress.  The session is not
  // deleted just in case that the user want to recover some files
  // from this session in the future.
  if (m_inProgress)
    m_inProgress->close();

  m_inProgress.reset();
}

void DataRecovery::updateConfig()
{
  auto& pref = Preferences::instance();
  m_config.dataRecoveryPeriod = pref.general.dataRecoveryPeriod();
  if (pref.general.keepEditedSpriteData())
    m_config.keepEditedSpriteDataFor = pref.general.keepEditedSpriteDataFor();
  else
    m_config.keepEditedSpriteDataFor = 0;

  // Wake up BackupObserver background thread to readjust the waiting period.
  if (m_backup)
    m_backup->wakeup();

  // Re-search to GC sessions (w/the new "keep edited sprite data for" period)
  if (m_thread.joinable()) {
    if (!m_searching)
      m_thread.join();
  }
  if (!m_thread.joinable()) {
    m_thread = std::thread([this] {
      base::this_thread::set_name("gc-sessions");
      gcSessions();
    });
  }
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

  m_thread = std::thread([this] {
    base::this_thread::set_name("search-sessions");
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
  for (const auto& itemname : base::list_files(m_sessionsDir, base::ItemType::Directories)) {
    const auto& itempath = base::join_path(m_sessionsDir, itemname);
    RECO_TRACE("RECO: Session '%s' ", itempath.c_str());

    const bool isRunningSession = (itempath == m_inProgress->path());
    if (!isRunningSession) {
      auto session = std::make_shared<Session>(&m_config, itempath, false);
      if ((session->isEmpty()) || (!session->isCrashedSession() && session->isOldSession())) {
        RECO_TRACE("to be deleted (%s)\n",
                   session->isEmpty() ? "is empty" :
                                        (session->isOldSession() ? "is old" : "unknown reason"));
        session->removeFromDisk();
      }
      else {
        RECO_TRACE("to be loaded\n");
        sessions.push_back(session);
      }
    }
    else
      RECO_TRACE("is running\n");
  }

  // Sort sessions from the most recent one to the oldest one
  std::sort(sessions.begin(), sessions.end(), [](const SessionPtr& a, const SessionPtr& b) {
    return a->name() > b->name();
  });

  // Assign m_sessions=sessions
  {
    std::unique_lock<std::mutex> lock(m_sessionsMutex);
    std::swap(m_sessions, sessions);
  }

  ui::execute_from_ui_thread([this] {
    if (g_stillAliveFlag)
      SessionsListIsReady();
  });
}

void DataRecovery::gcSessions()
{
  std::unique_lock<std::mutex> lock(m_sessionsMutex);
  RECO_TRACE("RECO: [BG] GC sessions\n");
  for (auto& s : m_sessions) {
    if (!s->isActiveSession() && (s->isEmpty() || (!s->isCrashedSession() && s->isOldSession()))) {
      RECO_TRACE("RECO: [BG] Remove session '%s' from disk (%s)\n",
                 s->name().c_str(),
                 s->isEmpty() ? "is empty" : (s->isOldSession() ? "is old" : "unknown reason"));
      s->removeFromDisk();
    }
  }
  RECO_TRACE("RECO: [BG] GC sessions ends\n");
}

}} // namespace app::crash
