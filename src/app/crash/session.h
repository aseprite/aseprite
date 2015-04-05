// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_CRASH_SESSION_H_INCLUDED
#define APP_CRASH_SESSION_H_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "base/process.h"
#include "base/shared_ptr.h"

#include <fstream>
#include <string>

namespace app {
namespace crash {

  // A class to record/restore session information.
  class Session {
  public:
    Session(const std::string& path);
    ~Session();

    bool isRunning();
    bool isEmpty();

    void create(base::pid pid);
    void removeFromDisk();

  private:
    void loadPid();
    std::string pidFilename() const;
    std::string dataFilename() const;

    base::pid m_pid;
    std::string m_path;
    std::fstream m_log;
    std::fstream m_pidFile;

    DISABLE_COPYING(Session);
  };

  typedef base::SharedPtr<Session> SessionPtr;

} // namespace crash
} // namespace app

#endif
