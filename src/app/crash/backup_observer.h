// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CRASH_BACKUP_OBSERVER_H_INCLUDED
#define APP_CRASH_BACKUP_OBSERVER_H_INCLUDED
#pragma once

#include "base/mutex.h"
#include "base/thread.h"
#include "doc/context_observer.h"
#include "doc/doc_observer.h"
#include "doc/documents_observer.h"

#include <vector>

namespace doc {
  class Context;
}

namespace app {
class Document;
namespace crash {
  class Session;

  class BackupObserver : public doc::ContextObserver
                       , public doc::DocumentsObserver
                       , public doc::DocObserver {
  public:
    BackupObserver(Session* session, doc::Context* ctx);
    ~BackupObserver();

    void stop();

    void onAddDocument(doc::Document* document) override;
    void onRemoveDocument(doc::Document* document) override;

  private:
    void backgroundThread();

    Session* m_session;
    base::mutex m_mutex;
    doc::Context* m_ctx;
    std::vector<app::Document*> m_documents;
    bool m_done;
    base::thread m_thread;
  };

} // namespace crash
} // namespace app

#endif
