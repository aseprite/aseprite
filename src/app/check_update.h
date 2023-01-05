// Aseprite
// Copyright (C) 2020-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CHECK_UPDATE_H_INCLUDED
#define APP_CHECK_UPDATE_H_INCLUDED
#pragma once

#ifdef ENABLE_UPDATER

#include "ui/timer.h"
#include "updater/check_update.h"

#include <atomic>
#include <memory>
#include <thread>

namespace app {

  class CheckUpdateDelegate;
  class CheckUpdateBackgroundJob;
  class Preferences;

  class CheckUpdateThreadLauncher {
  public:
    CheckUpdateThreadLauncher(CheckUpdateDelegate* delegate);
    ~CheckUpdateThreadLauncher();

    void launch();

    bool isReceived() const;

    const updater::CheckUpdateResponse& getResponse() const
    {
      return m_response;
    }

  private:
    void onMonitoringTick();
    void checkForUpdates();
    void showUI();

    CheckUpdateDelegate* m_delegate;
    Preferences& m_preferences;
    updater::Uuid m_uuid;
    std::unique_ptr<std::thread> m_thread;
    std::unique_ptr<CheckUpdateBackgroundJob> m_bgJob;
    bool m_doCheck;
    std::atomic<bool> m_received;

    // Mini-stats
    int m_inits;
    int m_exits;

    // True if this is a developer
    bool m_isDeveloper;

    updater::CheckUpdateResponse m_response;
    ui::Timer m_timer;
  };

} // namespace app

#endif // ENABLE_UPDATER

#endif // APP_CHECK_UPDATE_H_INCLUDED
