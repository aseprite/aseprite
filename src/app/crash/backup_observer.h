// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CRASH_BACKUP_OBSERVER_H_INCLUDED
#define APP_CRASH_BACKUP_OBSERVER_H_INCLUDED
#pragma once

#include "app/context_observer.h"
#include "app/doc_observer.h"
#include "app/docs_observer.h"
#include "base/mutex.h"
#include "base/thread.h"

#include <vector>

namespace app {
class Context;
class Doc;
namespace crash {
  struct RecoveryConfig;
  class Session;

  class BackupObserver : public ContextObserver
                       , public DocsObserver
                       , public DocObserver {
  public:
    BackupObserver(RecoveryConfig* config,
                   Session* session,
                   Context* ctx);
    ~BackupObserver();

    void stop();

    void onAddDocument(Doc* document) override;
    void onRemoveDocument(Doc* document) override;

  private:
    void backgroundThread();

    RecoveryConfig* m_config;
    Session* m_session;
    base::mutex m_mutex;
    Context* m_ctx;
    std::vector<Doc*> m_documents;
    bool m_done;
    base::thread m_thread;
  };

} // namespace crash
} // namespace app

#endif
