// Aseprite
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_RENDER_TASK_JOB_H_INCLUDED
#define APP_RENDER_TASK_JOB_H_INCLUDED
#pragma once

#include "app/job.h"
#include "render/task_delegate.h"

#include <functional>

namespace app {

class RenderTaskJob : public Job,
                      public render::TaskDelegate {
public:
  template<typename T>
  RenderTaskJob(const char* jobName, T&& func)
    : Job(jobName)
    , m_func(std::move(func)) {
  }

private:
  void onJob() override;

  // render::TaskDelegate impl
  bool continueTask() override;
  void notifyTaskProgress(double progress) override;

  std::function<void()> m_func;
};

} // namespace app

#endif
