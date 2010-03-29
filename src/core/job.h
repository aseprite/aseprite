/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#ifndef CORE_JOB_H_INCLUDED
#define CORE_JOB_H_INCLUDED

#include "jinete/jbase.h"

namespace Vaca { class Mutex; }

class Frame;
struct Monitor;
class Progress;

class Job
{
  JThread m_thread;
  Monitor* m_monitor;
  Progress* m_progress;
  Vaca::Mutex* m_mutex;
  Frame* m_alert_window;
  float m_last_progress;
  bool m_done_flag;
  bool m_canceled_flag;

  // these methods are privated and not defined
  Job();
  Job(const Job&);
  Job& operator==(const Job&);
  
public:

  Job(const char* job_name);
  virtual ~Job();

  void do_job();
  void job_progress(float f);
  bool is_canceled();

protected:

  virtual void on_job();
  virtual void on_monitor_tick();
  virtual void on_monitor_destroyed();

private:

  void done();

  static void thread_proc(void* data);
  static void monitor_proc(void* data);
  static void monitor_free(void* data);

};

#endif
