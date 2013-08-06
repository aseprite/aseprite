/* ASEPRITE
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

#include <stdlib.h>
#include <string.h>

#include "app/app.h"
#include "app/commands/filters/filter_manager_impl.h"
#include "app/ini_file.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/ui/editor/editor.h"
#include "app/ui/status_bar.h"
#include "base/mutex.h"
#include "base/scoped_lock.h"
#include "base/thread.h"
#include "raster/sprite.h"
#include "ui/ui.h"

namespace app {

using namespace base;
using namespace ui;
  
static const int kMonitoringPeriod = 100;

// Applies filters in two threads: a background worker thread to
// modify the sprite, and the main thread to monitoring the progress
// (and given to the user the possibility to cancel the process).

class FilterWorker : public FilterManagerImpl::IProgressDelegate
{
public:
  FilterWorker(FilterManagerImpl* filterMgr);
  ~FilterWorker();

  void run();

  // IProgressDelegate implementation
  void reportProgress(float progress);
  bool isCancelled();

private:
  void applyFilterInBackground();
  void onMonitoringTick();

  static void thread_proxy(void* data) {
    FilterWorker* filterWorker = (FilterWorker*)data;
    filterWorker->applyFilterInBackground();
  }

  FilterManagerImpl* m_filterMgr; // Effect to be applied.
  base::mutex m_mutex;          // Mutex to access to 'pos', 'done' and 'cancelled' fields in different threads.
  float m_pos;                  // Current progress position
  bool m_done : 1;              // Was the effect completelly applied?
  bool m_cancelled : 1;         // Was the effect cancelled by the user?
  ui::Timer m_timer;            // Monitoring timer to update the progress-bar
  Progress* m_progressBar;      // The progress-bar.
  AlertPtr m_alertWindow;       // Alert for the user to cancel the filter-progress if he wants.
};

FilterWorker::FilterWorker(FilterManagerImpl* filterMgr)
  : m_filterMgr(filterMgr)
  , m_timer(kMonitoringPeriod)
{
  m_filterMgr->setProgressDelegate(this);

  m_pos = 0.0;
  m_done = false;
  m_cancelled = false;

  m_progressBar = StatusBar::instance()->addProgress();

  m_alertWindow = ui::Alert::create(PACKAGE
                                    "<<Applying effect...||&Cancel");

  m_timer.Tick.connect(&FilterWorker::onMonitoringTick, this);
  m_timer.start();
}

FilterWorker::~FilterWorker()
{
  if (m_alertWindow != NULL)
    m_alertWindow->closeWindow(NULL);

  delete m_progressBar;
}

void FilterWorker::run()
{
  // Launch the thread to apply the effect in background
  base::thread thread(&FilterWorker::thread_proxy, this);

  // Open the alert window in foreground (this is modal, locks the main thread)
  m_alertWindow->openWindowInForeground();

  // Stop the monitoring timer.
  m_timer.stop();

  {
    scoped_lock lock(m_mutex);
    if (!m_done)
      m_cancelled = true;
  }

  // Wait the `effect_bg' thread
  thread.join();
}

// Called by FilterManagerImpl to informate the progress of the filter.
//
// [effect thread]
//
void FilterWorker::reportProgress(float progress)
{
  scoped_lock lock(m_mutex);
  m_pos = progress;
}

// Called by effect_apply to know if the user cancelled the operation.
//
// [effect thread]
//
bool FilterWorker::isCancelled()
{
  bool cancelled;

  scoped_lock lock(m_mutex);
  cancelled = m_cancelled;

  return cancelled;
}

// Applies the effect to the sprite in a background thread.
//
// [effect thread]
//
void FilterWorker::applyFilterInBackground()
{
  // Apply the filter
  m_filterMgr->applyToTarget();

  // Mark the work as 'done'.
  scoped_lock lock(m_mutex);
  m_done = true;
}

// Called by the GUI monitor (a timer in the gui module that is called
// every 100 milliseconds).
void FilterWorker::onMonitoringTick()
{
  scoped_lock lock(m_mutex);

  if (m_progressBar)
    m_progressBar->setPos(m_pos);

  if (m_done)
    m_alertWindow->closeWindow(NULL);
}

// Applies the filter in a background thread meanwhile a progress bar
// is shown to the user.
//
// [main thread]
//
void start_filter_worker(FilterManagerImpl* filterMgr)
{
  FilterWorker filterWorker(filterMgr);
  filterWorker.run();
}

} // namespace app
