// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CRASH_SESSION_H_INCLUDED
#define APP_CRASH_SESSION_H_INCLUDED
#pragma once

#include "app/crash/raw_images_as.h"
#include "base/disable_copying.h"
#include "base/process.h"
#include "base/shared_ptr.h"
#include "doc/object_id.h"

#include <fstream>
#include <string>
#include <vector>

namespace app {
class Doc;
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
    std::string version();
    const Backups& backups();

    bool isRunning();
    bool isEmpty();

    void create(base::pid pid);
    void removeFromDisk();

    bool saveDocumentChanges(Doc* doc);
    void removeDocument(Doc* doc);

    void restoreBackup(Backup* backup);
    void restoreBackupById(const doc::ObjectId id);
    Doc* restoreBackupDocById(const doc::ObjectId id);
    void restoreRawImages(Backup* backup, RawImagesAs as);
    void deleteBackup(Backup* backup);

  private:
    Doc* restoreBackupDoc(const std::string& backupDir);
    void loadPid();
    std::string pidFilename() const;
    std::string verFilename() const;
    void deleteDirectory(const std::string& dir);
    void fixFilename(Doc* doc);

    base::pid m_pid;
    std::string m_path;
    std::string m_version;
    Backups m_backups;

    DISABLE_COPYING(Session);
  };

  typedef base::SharedPtr<Session> SessionPtr;

} // namespace crash
} // namespace app

#endif
