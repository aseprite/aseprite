// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_JOB_H_INCLUDED
#define APP_JOB_H_INCLUDED
#pragma once

#include "ui/alert.h"
#include "ui/timer.h"

#include <exception>

namespace base {
  class thread;
  class mutex;
}

namespace app {

  class Job {
  public:
    static int runningJobs();

    Job(const char* jobName);
    virtual ~Job();

    // Starts the job calling onJob() event in another thread and
    // monitoring the progress with onMonitorTick() event.
    void startJob();

    void waitJob();

    // The onJob() can use this function to report progress of the
    // background job being done. 1.0 is completed.
    void jobProgress(double f);

    // Returns true if the job was canceled by the user (in case he
    // pressed a "Cancel" button in the GUI). The onJob() thread should
    // check this variable periodically to stop working.
    bool isCanceled();

  protected:

    // This member function is called from another dedicated thread
    // outside the GUI one, so you can do some image processing here.
    // Remember that you cannot use any GUI element in this handler.
    virtual void onJob() = 0;

    // Called each 1000 msecs by the GUI queue processing.
    // It is executed from the main GUI thread.
    virtual void onMonitoringTick();

  private:
    void done();

    static void thread_proc(Job* self);
    static void monitor_proc(void* data);
    static void monitor_free(void* data);

    base::thread* m_thread;
    std::unique_ptr<ui::Timer> m_timer;
    base::mutex* m_mutex;
    ui::AlertPtr m_alert_window;
    double m_last_progress;
    bool m_done_flag;
    bool m_canceled_flag;
    std::exception_ptr m_error;

    // these methods are privated and not defined
    Job();
    Job(const Job&);
    Job& operator==(const Job&);

  };

} // namespace app

#endif
