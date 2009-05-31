/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2009  David Capello
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

#include "jinete/jalert.h"
#include "jinete/jmutex.h"
#include "jinete/jthread.h"
#include "jinete/jwidget.h"
#include "jinete/jwindow.h"

#include "core/app.h"
#include "core/job.h"
#include "modules/gui.h"
#include "widgets/statebar.h"

Job::Job()
{
  m_mutex = NULL;
  m_thread = NULL;
  m_progress = NULL;
  m_monitor = NULL;
  m_alert_window = NULL;
  m_last_progress = 0.0f;
  m_done_flag = false;
  m_canceled_flag = false;

  m_mutex = jmutex_new();
  m_progress = progress_new(app_get_statusbar());
  m_monitor = add_gui_monitor(&Job::monitor_proc,
			      &Job::monitor_free,
			      (void*)this);
  m_alert_window = jalert_new(PACKAGE "<<Working...||&Cancel");
}

Job::~Job()
{
  // The job was canceled by the user?
  {
    ScopedLock hold(m_mutex);
    if (!m_done_flag)
      m_canceled_flag = true;
  }

  if (m_monitor) {
    remove_gui_monitor(m_monitor);
    m_monitor = NULL;
  }

  if (m_thread)
    jthread_join(m_thread);

  if (m_progress)
    progress_free(m_progress);

  if (m_mutex)
    jmutex_free(m_mutex);

  if (m_alert_window)
    jwidget_free(m_alert_window);
}

void Job::do_job()
{
  m_thread = jthread_new(&Job::thread_proc, (void*)this);
  jwindow_open_fg(m_alert_window);
}

void Job::job_progress(float f)
{
  ScopedLock hold(m_mutex);
  m_last_progress = f;
}

bool Job::is_canceled()
{
  ScopedLock hold(m_mutex);
  return m_canceled_flag;
}

/**
 * Called from another thread to do the hard work (image processing).
 * 
 * [working thread]
 */
void Job::on_job()
{
  // do nothing
}

/**
 * Called each 1000 msecs by the GUI queue processing.
 * 
 * [main thread]
 */
void Job::on_monitor_tick()
{
  ScopedLock hold(m_mutex);

  // update progress
  progress_update(m_progress, m_last_progress);

  // is job done? we can close the monitor
  if (m_done_flag)
    remove_gui_monitor(m_monitor);
}

/**
 * Called when the monitor is destroyed.
 *
 * [main thread]
 */
void Job::on_monitor_destroyed()
{
  if (m_alert_window != NULL) {
    m_monitor = NULL;
    jwindow_close(m_alert_window, NULL);
  }
}

void Job::done()
{
  ScopedLock hold(m_mutex);
  m_done_flag = true;
}

//////////////////////////////////////////////////////////////////////
// Static methods

/**
 * Called to start the worker thread.
 *
 * [worker thread]
 */
void Job::thread_proc(void* data)
{
  Job* self = (Job*)data;
  try {
    self->on_job();
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
  self->on_monitor_tick();
}

/**
 * Function called when the GUI monitor is deleted.
 *
 * [main thread]
 */
void Job::monitor_free(void* data)
{
  Job* self = (Job*)data;
  self->on_monitor_destroyed();
}
