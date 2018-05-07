// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SEND_CRASH_H_INCLUDED
#define APP_SEND_CRASH_H_INCLUDED
#pragma once

#include "app/notification_delegate.h"

#include <string>

namespace app {

  class SendCrash : public INotificationDelegate {
  public:
    void search();

    virtual std::string notificationText() override;
    virtual void notificationClick() override;

  private:
    void onClickFilename();
    void onClickDevFilename();

    std::string m_dumpFilename;
  };

} // namespace app

#endif // APP_SEND_CRASH_H_INCLUDED
