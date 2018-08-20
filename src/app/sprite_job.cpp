// Aseprite
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/sprite_job.h"

namespace app {

SpriteJob::SpriteJob(const ContextReader& reader, const char* jobName)
  : Job(jobName)
  , m_writer(reader, 500)
  , m_document(m_writer.document())
  , m_sprite(m_writer.sprite())
  , m_tx(m_writer.context(), jobName, ModifyDocument)
{
}

SpriteJob::~SpriteJob()
{
  if (!isCanceled())
    m_tx.commit();
}

void SpriteJob::onJob()
{
  m_callback();
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
