// Aseprite
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/render_task_job.h"

namespace app {

void RenderTaskJob::onJob()
{
  try {
    m_func();
  }
  catch (std::exception& ex) {
    // TODO show the exception
  }
}

bool RenderTaskJob::continueTask()
{
  return !isCanceled();
}

void RenderTaskJob::notifyTaskProgress(double progress)
{
  jobProgress(progress);
}

} // namespace app
