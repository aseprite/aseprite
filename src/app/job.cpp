// Aseprite
// Copyright (C) 2021-2024  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/job.h"

#include "app/app.h"
#include "app/console.h"
#include "app/context.h"
#include "app/i18n/strings.h"
#include "ui/alert.h"
#include "ui/widget.h"
#include "ui/window.h"

#include <atomic>

static const int kMonitoringPeriod = 100;
static std::atomic<int> g_runningJobs(0);

namespace app {

// static
int Job::runningJobs()
{
  return g_runningJobs;
}

Job::Job(const std::string& jobName, const bool showProgress)
{
  m_last_progress = 0.0;
  m_done_flag = false;
  m_canceled_flag = false;

  if (showProgress && App::instance()->isGui()) {
    m_alert_window = ui::Alert::create(Strings::alerts_job_working(jobName));
    m_alert_window->addProgress();

    m_timer = std::make_unique<ui::Timer>(kMonitoringPeriod, m_alert_window.get());
    m_timer->Tick.connect(&Job::onMonitoringTick, this);
    m_timer->start();
  }
}

Job::~Job()
{
  if (App::instance()->isGui()) {
    ASSERT(!m_timer || !m_timer->isRunning());

    if (m_alert_window)
      m_alert_window->closeWindow(NULL);
  }
}

void Job::startJob()
{
  m_thread = std::thread(&Job::thread_proc, this);
  ++g_runningJobs;

  if (m_alert_window) {
    m_alert_window->openWindowInForeground();

    // The job was canceled by the user?
    {
      std::unique_lock<std::mutex> hold(m_mutex);
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
      catch (...) {
        Console console;
        console.printf("Unknown error performing the task");
      }
    }
  }
}

void Job::waitJob()
{
  if (m_timer && m_timer->isRunning())
    m_timer->stop();

  if (m_thread.joinable()) {
    m_thread.join();

    --g_runningJobs;
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
  std::unique_lock<std::mutex> hold(m_mutex);

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
  std::unique_lock<std::mutex> hold(m_mutex);
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
