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

namespace app {

  class Task {
  public:
    Task();
    ~Task();

    void run(base::task::func_t&& func);
    void wait();

    // Returns true when the task is completed (whether it was
    // canceled or not)
    bool completed() const {
      return m_task.completed();
    }

    bool running() const {
      return m_task.running();
    }

    bool canceled() const {
      if (m_token)
        return m_token->canceled();
      else
        return false;
    }

    float progress() const {
      if (m_token)
        return m_token->progress();
      else
        return 0.0f;
    }

    void cancel() {
      if (m_token)
        m_token->cancel();
    }

    void set_progress(float progress) {
      if (m_token)
        m_token->set_progress(progress);
    }

  private:
    base::task m_task;
    base::task_token* m_token;
  };

} // namespace app

#endif
