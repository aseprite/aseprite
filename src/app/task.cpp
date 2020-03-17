// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/task.h"

#include "base/task.h"
#include "base/thread.h"
#include "base/thread_pool.h"

namespace app {

static base::thread_pool tasks_pool(4);

Task::Task()
  : m_token(nullptr)
{
}

Task::~Task()
{
}

void Task::run(base::task::func_t&& func)
{
  m_task.on_execute(std::move(func));
  m_token = &m_task.start(tasks_pool);
}

void Task::wait()
{
  // TODO wait a condition variable
  while (!m_task.completed()) {
    base::this_thread::sleep_for(0.1);
  }
}

} // namespace app
