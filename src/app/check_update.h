/* Aseprite
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef APP_CHECK_UPDATE_H_INCLUDED
#define APP_CHECK_UPDATE_H_INCLUDED
#pragma once

#ifdef ENABLE_UPDATER

#include "base/thread.h"
#include "base/unique_ptr.h"
#include "ui/timer.h"
#include "updater/check_update.h"

namespace app {

  class CheckUpdateBackgroundJob;

  class CheckUpdateThreadLauncher {
  public:
    CheckUpdateThreadLauncher();
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

    updater::Uuid m_uuid;
    base::UniquePtr<base::thread> m_thread;
    base::UniquePtr<CheckUpdateBackgroundJob> m_bgJob;
    bool m_doCheck;
    bool m_received;

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
