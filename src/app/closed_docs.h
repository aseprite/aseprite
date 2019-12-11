// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CLOSED_DOCS_H_INCLUDED
#define APP_CLOSED_DOCS_H_INCLUDED
#pragma once

#include "base/time.h"

#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

namespace app {

  class Doc;
  class Preferences;

  // Handle the list of closed docs:
  // * When a document is closed, we keep it for some time so the user
  //   can undo the close command without losing the undo history.
  // * For the first closed document, a thread is launched to wait
  //   until we can definitely delete the doc after X minutes (like a
  //   garbage collector).
  // * If the document was not restore, we delete it from memory, if
  //   the document was restore, we remove it from the m_docs.
  class ClosedDocs {
  public:
    ClosedDocs(const Preferences& pref);
    ~ClosedDocs();

    bool hasClosedDocs();
    void addClosedDoc(Doc* doc);
    Doc* reopenLastClosedDoc();

    // Called at the very end to get all closed docs, remove them from
    // the list of closed docs, and stop the thread.
    std::vector<Doc*> getAndRemoveAllClosedDocs();

  private:
    void backgroundThread();

    struct ClosedDoc {
      Doc* doc;
      base::tick_t timestamp;
    };

    bool m_done;
    base::tick_t m_dataRecoveryPeriodMSecs;
    base::tick_t m_keepClosedDocAliveForMSecs;
    std::vector<ClosedDoc> m_docs;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::thread m_thread;
  };

} // namespace app

#endif
