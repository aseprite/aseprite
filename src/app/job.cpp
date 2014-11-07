/* Aseprite
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/job.h"

#include "app/app.h"
#include "app/ui/status_bar.h"
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
  m_progress = NULL;
  m_last_progress = 0.0;
  m_done_flag = false;
  m_canceled_flag = false;

  m_mutex = new base::mutex();

  if (App::instance()->isGui()) {
    m_progress = StatusBar::instance()->addProgress();
    m_alert_window = ui::Alert::create("%s<<Working...||&Cancel", jobName);

    m_timer.reset(new ui::Timer(kMonitoringPeriod, m_alert_window));
    m_timer->Tick.connect(&Job::onMonitoringTick, this);
    m_timer->start();
  }
}

Job::~Job()
{
  if (App::instance()->isGui()) {
    ASSERT(!m_timer->isRunning());
    ASSERT(m_thread == NULL);

    if (m_alert_window != NULL)
      m_alert_window->closeWindow(NULL);

    if (m_progress)
      delete m_progress;
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
  base::scoped_lock hold(*m_mutex);
  m_last_progress = f;
}

bool Job::isCanceled()
{
  base::scoped_lock hold(*m_mutex);
  return m_canceled_flag;
}

void Job::onMonitoringTick()
{
  base::scoped_lock hold(*m_mutex);

  // update progress
  m_progress->setPos(m_last_progress);

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
    // TODO handle this exception
  }
  self->done();
}

} // namespace app
