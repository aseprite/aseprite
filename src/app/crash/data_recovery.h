// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CRASH_DATA_RECOVERY_H_INCLUDED
#define APP_CRASH_DATA_RECOVERY_H_INCLUDED
#pragma once

#include "app/crash/session.h"
#include "base/disable_copying.h"

#include <vector>

namespace doc {
  class Context;
}

namespace app {
namespace crash {
  class BackupObserver;

  class DataRecovery {
  public:
    typedef std::vector<SessionPtr> Sessions;

    DataRecovery(doc::Context* context);
    ~DataRecovery();

    Session* activeSession() { return m_inProgress.get(); }

    // Returns the list of sessions that can be recovered.
    const Sessions& sessions() { return m_sessions; }

  private:
    Sessions m_sessions;
    SessionPtr m_inProgress;
    BackupObserver* m_backup;

    DISABLE_COPYING(DataRecovery);
  };

} // namespace crash
} // namespace app

#endif
