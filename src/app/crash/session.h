// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
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
#include "base/task.h"
#include "doc/object_id.h"

#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace app {
class Doc;
namespace crash {
  struct RecoveryConfig;

  // A class to record/restore session information.
  class Session {
  public:
    class Backup {
    public:
      Backup(const std::string& dir);
      const std::string& dir() const { return m_dir; }
      std::string description(const bool withFullPath) const;
    private:
      std::string m_dir;
      std::string m_desc;
      std::string m_fn;
    };
    typedef std::vector<Backup*> Backups;

    Session(RecoveryConfig* config,
            const std::string& path);
    ~Session();

    std::string name() const;
    std::string version();
    const Backups& backups();

    bool isRunning();
    bool isCrashedSession();
    bool isOldSession();
    bool isEmpty();

    void create(base::pid pid);
    void close();
    void removeFromDisk();

    bool saveDocumentChanges(Doc* doc);
    void removeDocument(Doc* doc);

    Doc* restoreBackupDoc(Backup* backup,
                          base::task_token* t);
    Doc* restoreBackupById(const doc::ObjectId id, base::task_token* t);
    Doc* restoreBackupDocById(const doc::ObjectId id, base::task_token* t);
    Doc* restoreBackupRawImages(Backup* backup,
                                const RawImagesAs as, base::task_token* t);
    void deleteBackup(Backup* backup);

  private:
    Doc* restoreBackupDoc(const std::string& backupDir,
                          base::task_token* t);
    void loadPid();
    std::string pidFilename() const;
    std::string verFilename() const;
    void markDocumentAsCorrectlyClosed(Doc* doc);
    void deleteDirectory(const std::string& dir);
    void fixFilename(Doc* doc);

    base::pid m_pid;
    std::string m_path;
    std::string m_version;
    Backups m_backups;
    RecoveryConfig* m_config;

    DISABLE_COPYING(Session);
  };

  typedef std::shared_ptr<Session> SessionPtr;

} // namespace crash
} // namespace app

#endif
