// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SEND_CRASH_H_INCLUDED
#define APP_SEND_CRASH_H_INCLUDED
#pragma once

#include "app/notification_delegate.h"
#include "app/task.h"

#include <string>

namespace app {

  class SendCrash
#ifdef ENABLE_UI
    : public INotificationDelegate
#endif
  {
  public:
    static std::string DefaultMemoryDumpFilename();

    ~SendCrash();

    void search();

#ifdef ENABLE_UI
  public:                       // INotificationDelegate impl
    virtual std::string notificationText() override;
    virtual void notificationClick() override;

  private:
    void onClickFilename();
    void onClickDevFilename();
#endif // ENABLE_UI

  private:
    Task m_task;
    std::string m_dumpFilename;
  };

} // namespace app

#endif // APP_SEND_CRASH_H_INCLUDED
