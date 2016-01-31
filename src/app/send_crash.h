// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

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

  std::string memory_dump_filename();

} // namespace app

#endif // APP_SEND_CRASH_H_INCLUDED
