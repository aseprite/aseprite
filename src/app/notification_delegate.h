// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_NOTIFICATION_DELEGATE_H_INCLUDED
#define APP_NOTIFICATION_DELEGATE_H_INCLUDED
#pragma once

#include <string>

namespace app {

  class INotificationDelegate {
  public:
    virtual ~INotificationDelegate() { }
    virtual std::string notificationText() = 0;
    virtual void notificationClick() = 0;
  };

}

#endif
