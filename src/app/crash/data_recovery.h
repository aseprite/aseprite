// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CRASH_DATA_RECOVERY_H_INCLUDED
#define APP_CRASH_DATA_RECOVERY_H_INCLUDED
#pragma once

#include "app/crash/recovery_config.h"
#include "app/crash/session.h"
#include "base/disable_copying.h"
#include "obs/signal.h"

#include <mutex>
#include <thread>
#include <vector>

namespace app {
class Context;
namespace crash {
  class BackupObserver;

  class DataRecovery {
  public:
    typedef std::vector<SessionPtr> Sessions;

    DataRecovery(Context* context);
    ~DataRecovery();

    // Launches the thread to search for sessions.
    void launchSearch();

    bool isSearching() const { return m_searching; }

    // Returns true if there is at least one sessions with sprites to
    // recover (i.e. a crashed session were changes weren't saved)
    bool hasRecoverySessions() const;

    Session* activeSession() { return m_inProgress.get(); }

    // Returns a copy of the list of sessions that can be recovered.
    Sessions sessions();

    // Triggered in the UI-thread from the m_thread using an
    // ui::execute_from_ui_thread() when the list of sessions is ready
    // to be used.
    obs::signal<void()> SessionsListIsReady;

  private:
    // Executed from m_thread to search for the list of sessions.
    void searchForSessions();

    std::string m_sessionsDir;
    mutable std::mutex m_sessionsMutex;
    std::thread m_thread;
    RecoveryConfig m_config;
    Sessions m_sessions;
    SessionPtr m_inProgress;
    BackupObserver* m_backup;
    bool m_searching;

    DISABLE_COPYING(DataRecovery);
  };

} // namespace crash
} // namespace app

#endif
