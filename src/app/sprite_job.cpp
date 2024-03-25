// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/sprite_job.h"

#include "base/log.h"

namespace app {

SpriteJob::SpriteJob(Context* ctx, Doc* doc,
                     const std::string& jobName)
  : Job(jobName)
  , m_doc(doc)
  , m_sprite(doc->sprite())
  , m_tx(Tx::DontLockDoc, ctx, doc, jobName, ModifyDocument)
  , m_lockAction(Tx::LockDoc)
{
  // Try to write-lock the document to see if we have to lock the
  // document in the background thread.
  auto lockResult = m_doc->writeLock(500);
  if (lockResult != Doc::LockResult::Fail) {
    if (lockResult == Doc::LockResult::Reentrant)
      m_lockAction = Tx::DontLockDoc;
    m_doc->unlock(lockResult);
  }
}

SpriteJob::~SpriteJob()
{
  try {
    if (!isCanceled())
      m_tx.commit();
  }
  catch (const std::exception& ex) {
    LOG(ERROR, "Error committing changes: %s\n", ex.what());
  }
}

void SpriteJob::onSpriteJob(Tx& tx)
{
  if (m_callback)
    m_callback(tx);
}

void SpriteJob::onJob()
{
  Tx subtx(m_lockAction, m_ctx, m_doc);
  onSpriteJob(subtx);
}

bool SpriteJob::continueTask()
{
  return !isCanceled();
}

void SpriteJob::notifyTaskProgress(double progress)
{
  jobProgress(progress);
}

} // namespace app
