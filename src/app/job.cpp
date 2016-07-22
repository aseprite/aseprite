// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/job.h"

#include "app/app.h"
#include "app/console.h"
#include "base/mutex.h"
#include "base/scoped_lock.h"
#include "base/thread.h"
#include "ui/alert.h"
#include "ui/widget.h"
#include "ui/window.h"

static const int kMonitoringPeriod = 100;

namespace app {

Job::Job(const char* jobName)
{
  m_mutex = NULL;
  m_thread = NULL;
  m_last_progress = 0.0;
  m_done_flag = false;
  m_canceled_flag = false;

  m_mutex = new base::mutex();

  if (App::instance()->isGui()) {
    m_alert_window = ui::Alert::create("%s<<Working...||&Cancel", jobName);
    m_alert_window->addProgress();

    m_timer.reset(new ui::Timer(kMonitoringPeriod, m_alert_window.get()));
    m_timer->Tick.connect(&Job::onMonitoringTick, this);
    m_timer->start();
  }
}

Job::~Job()
{
  if (App::instance()->isGui()) {
    ASSERT(!m_timer->isRunning());
    ASSERT(m_thread == NULL);

    if (m_alert_window)
      m_alert_window->closeWindow(NULL);
  }

  if (m_mutex)
    delete m_mutex;
}

void Job::startJob()
{
  m_thread = new base::thread(&Job::thread_proc, this);

  if (m_alert_window) {
    m_alert_window->openWindowInForeground();

    // The job was canceled by the user?
    {
      base::scoped_lock hold(*m_mutex);
      if (!m_done_flag)
        m_canceled_flag = true;
    }

    // In case of error, take the "cancel" path (i.e. it's like the
    // user canceled the operation).
    if (m_error) {
      m_canceled_flag = true;
      try {
        std::rethrow_exception(m_error);
      }
      catch (const std::exception& ex) {
        Console::showException(ex);
      }
    }
  }
}

void Job::waitJob()
{
  if (m_timer && m_timer->isRunning())
    m_timer->stop();

  if (m_thread) {
    m_thread->join();
    delete m_thread;
    m_thread = NULL;
  }
}

void Job::jobProgress(double f)
{
  m_last_progress = f;
}

bool Job::isCanceled()
{
  return m_canceled_flag;
}

void Job::onMonitoringTick()
{
  base::scoped_lock hold(*m_mutex);

  // update progress
  m_alert_window->setProgress(m_last_progress);

  // is job done? we can close the monitor
  if (m_done_flag || m_canceled_flag) {
    m_timer->stop();
    m_alert_window->closeWindow(NULL);
  }
}

void Job::done()
{
  base::scoped_lock hold(*m_mutex);
  m_done_flag = true;
}

// Called to start the worker thread.
void Job::thread_proc(Job* self)
{
  try {
    self->onJob();
  }
  catch (...) {
    self->m_error = std::current_exception();
  }
  self->done();
}

} // namespace app
