// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_TASK_H_INCLUDED
#define APP_TASK_H_INCLUDED
#pragma once

#include "base/task.h"

#include <functional>
#include <mutex>

namespace app {

class Task {
public:
  Task();
  ~Task();

  void run(base::task::func_t&& func);
  void wait();

  // Returns true when the task is completed (whether it was
  // canceled or not)
  bool completed() const { return m_task.completed(); }

  bool running() const { return m_task.running(); }

  bool canceled() const
  {
    const std::lock_guard lock(m_token_mutex);
    if (m_token)
      return m_token->canceled();
    return false;
  }

  float progress() const
  {
    const std::lock_guard lock(m_token_mutex);
    if (m_token)
      return m_token->progress();
    return 0.0f;
  }

  void cancel()
  {
    const std::lock_guard lock(m_token_mutex);
    if (m_token)
      m_token->cancel();
  }

  void set_progress(float progress)
  {
    const std::lock_guard lock(m_token_mutex);
    if (m_token)
      m_token->set_progress(progress);
  }

private:
  base::task m_task;
  mutable std::mutex m_token_mutex;
  base::task_token* m_token;
};

} // namespace app

#endif
