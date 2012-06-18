/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#include "config.h"

#include "app.h"
#include "base/mutex.h"
#include "base/scoped_lock.h"
#include "base/thread.h"
#include "gui/alert.h"
#include "gui/frame.h"
#include "gui/widget.h"
#include "job.h"
#include "modules/gui.h"
#include "widgets/statebar.h"

Job::Job(const char* job_name)
{
  m_mutex = NULL;
  m_thread = NULL;
  m_progress = NULL;
  m_monitor = NULL;
  m_last_progress = 0.0f;
  m_done_flag = false;
  m_canceled_flag = false;

  m_mutex = new Mutex();
  m_progress = app_get_statusbar()->addProgress();
  m_monitor = add_gui_monitor(&Job::monitor_proc,
                              &Job::monitor_free,
                              (void*)this);
  m_alert_window = ui::Alert::create("%s<<Working...||&Cancel", job_name);
}

Job::~Job()
{
  // The job was canceled by the user?
  {
    ScopedLock hold(*m_mutex);
    if (!m_done_flag)
      m_canceled_flag = true;
  }

  if (m_monitor) {
    remove_gui_monitor(m_monitor);
    m_monitor = NULL;
  }

  if (m_thread) {
    m_thread->join();
    delete m_thread;
  }

  if (m_progress)
    delete m_progress;

  if (m_mutex)
    delete m_mutex;
}

void Job::startJob()
{
  m_thread = new base::thread(&Job::thread_proc, this);
  m_alert_window->open_window_fg();
}

void Job::jobProgress(float f)
{
  ScopedLock hold(*m_mutex);
  m_last_progress = f;
}

bool Job::isCanceled()
{
  ScopedLock hold(*m_mutex);
  return m_canceled_flag;
}

void Job::onJob()
{
  // do nothing
}

void Job::onMonitorTick()
{
  ScopedLock hold(*m_mutex);

  // update progress
  m_progress->setPos(m_last_progress);

  // is job done? we can close the monitor
  if (m_done_flag)
    remove_gui_monitor(m_monitor);
}

void Job::onMonitorDestroyed()
{
  if (m_alert_window != NULL) {
    m_monitor = NULL;
    m_alert_window->closeWindow(NULL);
  }
}

void Job::done()
{
  ScopedLock hold(*m_mutex);
  m_done_flag = true;
}

//////////////////////////////////////////////////////////////////////
// Static methods

/**
 * Called to start the worker thread.
 *
 * [worker thread]
 */
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

/**
 * Procedure called from the GUI loop to monitoring each 100 milliseconds.
 *
 * [main thread]
 */
void Job::monitor_proc(void* data)
{
  Job* self = (Job*)data;
  self->onMonitorTick();
}

/**
 * Function called when the GUI monitor is deleted.
 *
 * [main thread]
 */
void Job::monitor_free(void* data)
{
  Job* self = (Job*)data;
  self->onMonitorDestroyed();
}
