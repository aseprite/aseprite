// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_CHECK_UPDATE_DELEGATE_H_INCLUDED
#define APP_CHECK_UPDATE_DELEGATE_H_INCLUDED
#pragma once

#ifdef ENABLE_UPDATER

#include <string>

namespace app {

  class CheckUpdateDelegate {
  public:
    virtual ~CheckUpdateDelegate() { }
    virtual void onCheckingUpdates() = 0;
    virtual void onUpToDate() = 0;
    virtual void onNewUpdate(const std::string& url, const std::string& version) = 0;
  };

} // namespace app

#endif // ENABLE_UPDATER

#endif // APP_CHECK_UPDATE_DELEGATE_H_INCLUDED
