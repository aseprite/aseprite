// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

// Uncomment if you want to test the backup process each 5 secondsh
//#define TEST_BACKUPS_WITH_A_SHORT_PERIOD

// Uncomment if you want to check that backups are correctly saved
// after being saved.
//#define TEST_BACKUP_INTEGRITY

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/crash/backup_observer.h"

#include "app/app.h"
#include "app/context.h"
#include "app/crash/session.h"
#include "app/doc.h"
#include "app/doc_access.h"
#include "app/doc_diff.h"
#include "app/pref/preferences.h"
#include "base/bind.h"
#include "base/chrono.h"
#include "base/remove_from_container.h"
#include "base/scoped_lock.h"
#include "ui/system.h"

namespace app {
namespace crash {

namespace {

class SwitchBackupIcon {
public:
  SwitchBackupIcon() {
    ui::execute_from_ui_thread(
      []{
        if (App* app = App::instance())
          app->showBackupNotification(true);
      });
  }
  ~SwitchBackupIcon() {
    ui::execute_from_ui_thread(
      []{
        if (App* app = App::instance())
          app->showBackupNotification(false);
      });
  }
};

}

BackupObserver::BackupObserver(Session* session, Context* ctx)
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

void BackupObserver::onAddDocument(Doc* document)
{
  TRACE("RECO: Observe document %p\n", document);
  base::scoped_lock hold(m_mutex);
  m_documents.push_back(document);
}

void BackupObserver::onRemoveDocument(Doc* document)
{
  TRACE("RECO: Remove document %p\n", document);
  {
    base::scoped_lock hold(m_mutex);
    base::remove_from_container(m_documents, document);
  }
  m_session->removeDocument(document);
}

void BackupObserver::backgroundThread()
{
  int normalPeriod = int(60.0*Preferences::instance().general.dataRecoveryPeriod());
  int lockedPeriod = 5;
#ifdef TEST_BACKUPS_WITH_A_SHORT_PERIOD
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

      for (Doc* doc : m_documents) {
        try {
          if (doc->needsBackup()) {
            if (doc->inhibitBackup()) {
              TRACE("RECO: Document '%d' backup is temporarily inhibited\n", doc->id());
              somethingLocked = true;
            }
            else if (!m_session->saveDocumentChanges(doc)) {
              TRACE("RECO: Document '%d' backup was canceled by UI\n", doc->id());
              somethingLocked = true;
            }
#ifdef TEST_BACKUP_INTEGRITY
            else {
              DocReader reader(doc, 500);
              std::unique_ptr<Doc> copy(
                m_session->restoreBackupDocById(doc->id()));
              DocDiff diff = compare_docs(doc, copy.get());
              if (diff.anything) {
                TRACE("RECO: Differences (%s/%s/%s/%s/%s/%s/%s)\n",
                      diff.canvas ? "canvas": "",
                      diff.totalFrames ? "totalFrames": "",
                      diff.frameDuration ? "frameDuration": "",
                      diff.frameTags ? "frameTags": "",
                      diff.palettes ? "palettes": "",
                      diff.layers ? "layers": "",
                      diff.cels ? "cels": "",
                      diff.images ? "images": "",
                      diff.colorProfiles ? "colorProfiles": "");

                Doc* copyDoc = copy.release();
                ui::execute_from_ui_thread(
                  [this, copyDoc] {
                    m_ctx->documents().add(copyDoc);
                  });
              }
              else {
                TRACE("RECO: No differences\n");
              }
            }
#endif
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
