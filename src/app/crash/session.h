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
#include <vector>

namespace app {
  class Document;

namespace crash {

  // A class to record/restore session information.
  class Session {
  public:
    class Backup {
    public:
      Backup(const std::string& dir);
      const std::string& dir() const { return m_dir; }
      const std::string& description() const { return m_desc; }
    private:
      std::string m_dir;
      std::string m_desc;
    };
    typedef std::vector<Backup*> Backups;

    Session(const std::string& path);
    ~Session();

    std::string name() const;
    const Backups& backups();

    bool isRunning();
    bool isEmpty();

    void create(base::pid pid);
    void removeFromDisk();

    void saveDocumentChanges(app::Document* doc);
    void removeDocument(app::Document* doc);

    void restoreBackup(Backup* backup);
    void deleteBackup(Backup* backup);

  private:
    void loadPid();
    std::string pidFilename() const;
    void deleteDirectory(const std::string& dir);

    base::pid m_pid;
    std::string m_path;
    std::fstream m_log;
    std::fstream m_pidFile;
    Backups m_backups;

    DISABLE_COPYING(Session);
  };

  typedef base::SharedPtr<Session> SessionPtr;

} // namespace crash
} // namespace app

#endif
