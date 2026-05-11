// Aseprite
// Copyright (C) 2019-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/closed_docs.h"
#include "app/doc.h"
#include "app/pref/preferences.h"
#include "base/thread.h"
#include "ui/system.h"

#include <algorithm>
#include <limits>

#define CLOSEDOC_TRACE(...) // TRACEARGS(__VA_ARGS__)

namespace app {

ClosedDocs::ClosedDocs(const Preferences& pref) : m_done(false)
{
  if (pref.general.dataRecovery())
    m_dataRecoveryPeriodMSecs = int(1000.0 * 60.0 * pref.general.dataRecoveryPeriod());
  else
    m_dataRecoveryPeriodMSecs = 0;

  if (pref.general.keepClosedSpriteOnMemory()) {
    if (pref.general.keepClosedSpriteOnMemoryFor() < 0)
      m_keepClosedDocAliveForMSecs = std::numeric_limits<base::tick_t>::max();
    else
      m_keepClosedDocAliveForMSecs = int(1000.0 * 60.0 *
                                         pref.general.keepClosedSpriteOnMemoryFor());
  }
  else
    m_keepClosedDocAliveForMSecs = 0;

  CLOSEDOC_TRACE("CLOSEDOC: Init",
                 "dataRecoveryPeriod",
                 m_dataRecoveryPeriodMSecs,
                 "keepClosedDocs",
                 m_keepClosedDocAliveForMSecs);
}

ClosedDocs::~ClosedDocs()
{
  CLOSEDOC_TRACE("CLOSEDOC: Exit");

  if (m_thread.joinable()) {
    CLOSEDOC_TRACE("CLOSEDOC: Join thread");

    m_done = true;
    m_cv.notify_one();
    m_thread.join();

    CLOSEDOC_TRACE("CLOSEDOC: Join done");
  }

  ASSERT(m_docs.empty());
}

bool ClosedDocs::hasClosedDocs()
{
  bool result;
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    result = !m_docs.empty();
  }
  CLOSEDOC_TRACE("CLOSEDOC: Has closed docs?", (result ? "true" : "false"));
  return result;
}

void ClosedDocs::addClosedDoc(Doc* doc)
{
  CLOSEDOC_TRACE("CLOSEDOC: Add closed doc", doc);

  ASSERT(doc != nullptr);
  ASSERT(doc->context() == nullptr);

  ClosedDoc closedDoc = { doc, base::current_tick() };
  ObjectId docId = doc->id();

  {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_docs.insert(m_docs.begin(), std::move(closedDoc));

    if (!m_thread.joinable())
      m_thread = std::thread([this] { backgroundThread(); });
    else
      m_cv.notify_one();
  }

  notify_observers(&ClosedDocsObserver::onNewClosedDoc, docId);
}

Doc* ClosedDocs::reopenLastClosedDoc()
{
  Doc* doc = nullptr;
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_docs.empty()) {
      doc = m_docs.front().doc;
      m_docs.erase(m_docs.begin());
    }
    CLOSEDOC_TRACE(" -> ", doc);
  }
  CLOSEDOC_TRACE("CLOSEDOC: Reopen last closed doc", doc);
  if (doc)
    notify_observers(&ClosedDocsObserver::onReopenClosedDoc, doc->id());
  return doc;
}

Doc* ClosedDocs::reopenClosedDocById(const doc::ObjectId docId)
{
  Doc* doc = nullptr;
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    for (auto it = m_docs.begin(), end = m_docs.end(); it != end; ++it) {
      if (it->doc && it->doc->id() == docId) {
        doc = it->doc;
        m_docs.erase(it);
        break;
      }
    }
  }
  CLOSEDOC_TRACE("CLOSEDOC: Reopen specific closed doc", doc);
  if (doc)
    notify_observers(&ClosedDocsObserver::onReopenClosedDoc, doc->id());
  return doc;
}

std::vector<crash::DocumentInfo> ClosedDocs::getClosedDocInfos()
{
  std::vector<crash::DocumentInfo> infos;
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    for (const ClosedDoc& closedDoc : m_docs)
      infos.push_back(crash::DocumentInfo(closedDoc.doc));
  }
  return infos;
}

std::vector<Doc*> ClosedDocs::getAndRemoveAllClosedDocs()
{
  std::vector<Doc*> docs;
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    CLOSEDOC_TRACE("CLOSEDOC: Get and remove all closed", m_docs.size(), "docs");
    for (const ClosedDoc& closedDoc : m_docs)
      docs.push_back(closedDoc.doc);
    m_docs.clear();
    m_done = true;
    m_cv.notify_one();
  }
  return docs;
}

void ClosedDocs::backgroundThread()
{
  CLOSEDOC_TRACE("CLOSEDOC: [BG] Background thread start");

  base::this_thread::set_name("closed-docs");

  std::unique_lock<std::mutex> lock(m_mutex);
  while (!m_done) {
    base::tick_t now = base::current_tick();
    base::tick_t waitForMSecs = std::numeric_limits<base::tick_t>::max();

    if (m_keepClosedDocAliveForMSecs == std::numeric_limits<base::tick_t>::max()) {
      m_done = true;
      continue;
    }

    for (auto it = m_docs.begin(); it != m_docs.end();) {
      const ClosedDoc& closedDoc = *it;
      auto doc = closedDoc.doc;

      base::tick_t diff = now - closedDoc.timestamp;
      if (diff >= m_keepClosedDocAliveForMSecs) {
        if ( // If the backup process is disabled
          m_dataRecoveryPeriodMSecs == 0 ||
          // Or this document doesn't need a backup (e.g. an unmodified document)
          !doc->needsBackup() ||
          // Or the document already has the backup done
          doc->isFullyBackedUp()) {
          const bool backedup = (doc->needsBackup() && doc->isFullyBackedUp());

          // Finally delete the document (this is the place where we
          // delete all documents created/loaded by the user)
          CLOSEDOC_TRACE("CLOSEDOC: [BG] Delete doc", doc);
          const doc::ObjectId docId = doc->id();
          delete doc;
          it = m_docs.erase(it);

          ui::execute_from_ui_thread([this, docId, backedup] {
            notify_observers(&ClosedDocsObserver::onArchiveClosedDoc, docId, backedup);
          });
        }
        else {
          waitForMSecs = std::min(waitForMSecs, m_dataRecoveryPeriodMSecs);
          ++it;
        }
      }
      else {
        waitForMSecs = std::min(waitForMSecs, m_keepClosedDocAliveForMSecs - diff);
        ++it;
      }
    }

    if (waitForMSecs < std::numeric_limits<base::tick_t>::max()) {
      CLOSEDOC_TRACE("CLOSEDOC: [BG] Wait for", waitForMSecs, "milliseconds");

      ASSERT(!m_docs.empty());
      m_cv.wait_for(lock, std::chrono::milliseconds(waitForMSecs));
    }
    else {
      CLOSEDOC_TRACE("CLOSEDOC: [BG] Wait for condition variable");

      ASSERT(m_docs.empty());
      m_cv.wait(lock);
    }
  }

  CLOSEDOC_TRACE("CLOSEDOC: [BG] Background thread end");
}

} // namespace app
