// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/crash/backup_observer.h"

#include "app/app.h"
#include "app/crash/session.h"
#include "app/document.h"
#include "app/pref/preferences.h"
#include "base/bind.h"
#include "base/chrono.h"
#include "base/remove_from_container.h"
#include "base/scoped_lock.h"
#include "doc/context.h"

namespace app {
namespace crash {

namespace {

class SwitchBackupIcon {
public:
  SwitchBackupIcon() {
    App* app = App::instance();
    if (app)
      app->showBackupNotification(true);
  }
  ~SwitchBackupIcon() {
    App* app = App::instance();
    if (app)
      app->showBackupNotification(false);
  }
};

}

BackupObserver::BackupObserver(Session* session, doc::Context* ctx)
  : m_session(session)
  , m_ctx(ctx)
  , m_done(false)
  , m_thread(base::Bind<void>(&BackupObserver::backgroundThread, this))
{
  m_ctx->add_observer(this);
  m_ctx->documents().add_observer(this);
}

BackupObserver::~BackupObserver()
{
  m_thread.join();
  m_ctx->documents().remove_observer(this);
  m_ctx->remove_observer(this);
}

void BackupObserver::stop()
{
  m_done = true;
}

void BackupObserver::onAddDocument(doc::Document* document)
{
  TRACE("RECO: Observe document %p\n", document);
  base::scoped_lock hold(m_mutex);
  m_documents.push_back(static_cast<app::Document*>(document));
}

void BackupObserver::onRemoveDocument(doc::Document* document)
{
  TRACE("RECO:: Remove document %p\n", document);
  {
    base::scoped_lock hold(m_mutex);
    base::remove_from_container(m_documents, static_cast<app::Document*>(document));
  }
  m_session->removeDocument(static_cast<app::Document*>(document));
}

void BackupObserver::backgroundThread()
{
  int normalPeriod = int(60.0*Preferences::instance().general.dataRecoveryPeriod());
  int lockedPeriod = 5;
#if 0                           // Just for testing purposes
  normalPeriod = 5;
  lockedPeriod = 5;
#endif

  int waitUntil = normalPeriod;
  int seconds = 0;

  while (!m_done) {
    seconds++;
    if (seconds >= waitUntil) {
      TRACE("RECO: Start backup process for %d documents\n", m_documents.size());

      SwitchBackupIcon icon;
      base::scoped_lock hold(m_mutex);
      base::Chrono chrono;
      bool somethingLocked = false;

      for (app::Document* doc : m_documents) {
        try {
          if (doc->needsBackup()) {
            if (!m_session->saveDocumentChanges(doc)) {
              TRACE("RECO: Document '%d' backup was canceled by UI\n", doc->id());
              somethingLocked = true;
            }
          }
        }
        catch (const std::exception&) {
          TRACE("RECO: Document '%d' is locked\n", doc->id());
          somethingLocked = true;
        }
      }

      seconds = 0;
      waitUntil = (somethingLocked ? lockedPeriod: normalPeriod);

      TRACE("RECO: Backup process done (%.16g)\n", chrono.elapsed());
    }
    base::this_thread::sleep_for(1.0);
  }
}

} // namespace crash
} // namespace app
