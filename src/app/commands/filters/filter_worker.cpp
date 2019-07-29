// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/filters/filter_manager_impl.h"
#include "app/console.h"
#include "app/context_access.h"
#include "app/i18n/strings.h"
#include "app/ini_file.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/ui/editor/editor.h"
#include "app/ui/status_bar.h"
#include "base/mutex.h"
#include "base/scoped_lock.h"
#include "base/thread.h"
#include "doc/sprite.h"
#include "ui/ui.h"

#include <cstdlib>
#include <cstring>
#include <functional>
#include <thread>

namespace app {

using namespace base;
using namespace ui;

#ifdef ENABLE_UI

namespace {

const int kMonitoringPeriod = 100;

class FilterWorkerAlert {
public:
  FilterWorkerAlert(std::function<void()>&& onTick)
    : m_timer(kMonitoringPeriod)
    , m_window(ui::Alert::create(Strings::alerts_applying_filter())) {
    m_window->addProgress();

    m_timer.Tick.connect(std::move(onTick));
    m_timer.start();
  }

  void openAndWait() {
    m_window->openWindowInForeground();

    // Stop the monitoring timer.
    m_timer.stop();
  }

  void close() {
    m_window->closeWindow(nullptr);
  }

  void setProgress(double progress) {
    m_window->setProgress(progress);
  }

private:
  ui::Timer m_timer; // Monitoring timer to update the progress-bar
  AlertPtr m_window; // Alert for the user to cancel the filter-progress if he wants.
};

} // anonymous namespace

#endif  // ENABLE_UI

// Applies filters in two threads: a background worker thread to
// modify the sprite, and the main thread to monitoring the progress
// (and given to the user the possibility to cancel the process).

class FilterWorker : public FilterManagerImpl::IProgressDelegate {
public:
  FilterWorker(FilterManagerImpl* filterMgr);
  ~FilterWorker();

  void run();

  // IProgressDelegate implementation
  void reportProgress(float progress);
  bool isCancelled();

private:
  void applyFilterInBackground();
#ifdef ENABLE_UI
  void onMonitoringTick();
#endif

  FilterManagerImpl* m_filterMgr; // Effect to be applied.
  base::mutex m_mutex;          // Mutex to access to 'pos', 'done' and 'cancelled' fields in different threads.
  float m_pos;                  // Current progress position
  bool m_done;                  // Was the effect completely applied?
  bool m_cancelled;             // Was the effect cancelled by the user?
  bool m_abort;                 // An exception was thrown
  std::string m_error;
#ifdef ENABLE_UI
  std::unique_ptr<FilterWorkerAlert> m_alert;
#endif
};

FilterWorker::FilterWorker(FilterManagerImpl* filterMgr)
  : m_filterMgr(filterMgr)
{
  m_filterMgr->setProgressDelegate(this);

  m_pos = 0.0;
  m_done = false;
  m_cancelled = false;
  m_abort = false;

#ifdef ENABLE_UI
  if (Manager::getDefault())
    m_alert.reset(new FilterWorkerAlert([this]{ onMonitoringTick(); }));
#endif
}

FilterWorker::~FilterWorker()
{
#ifdef ENABLE_UI
  if (m_alert)
    m_alert->close();
#endif
}

void FilterWorker::run()
{
  // Initialize writting transaction
  m_filterMgr->initTransaction();

#ifdef ENABLE_UI
  std::thread thread;
  // Open the alert window in foreground (this is modal, locks the main thread)
  if (m_alert) {
    // Launch the thread to apply the effect in background
    thread = std::thread([this]{ applyFilterInBackground(); });
    m_alert->openAndWait();
  }
  else
#endif // ENABLE_UI
  {
    // Without UI? Apply filter from the main thread
    applyFilterInBackground();
  }

  {
    scoped_lock lock(m_mutex);
    if (m_done && m_filterMgr->isTransaction())
      m_filterMgr->commitTransaction();
    else
      m_cancelled = true;
  }

#ifdef ENABLE_UI
  // Wait the `effect_bg' thread
  if (thread.joinable())
    thread.join();

  if (!m_error.empty()) {
    Console console;
    console.printf("A problem has occurred.\n\nDetails:\n%s", m_error.c_str());
  }
  else if (m_cancelled && !m_filterMgr->isTransaction()) {
    StatusBar::instance()
      ->showTip(2500, "No unlocked layers to apply filter");
  }
#endif // ENABLE_UI
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
  cancelled = (m_cancelled || m_abort);

  return cancelled;
}

// Applies the effect to the sprite in a background thread.
//
// [effect thread]
//
void FilterWorker::applyFilterInBackground()
{
  try {
    // Apply the filter
    m_filterMgr->applyToTarget();

    // Mark the work as 'done'.
    scoped_lock lock(m_mutex);
    m_done = true;
  }
  catch (std::exception& e) {
    m_error = e.what();
    m_abort = true;
  }
}

#ifdef ENABLE_UI

// Called by the GUI monitor (a timer in the gui module that is called
// every 100 milliseconds).
void FilterWorker::onMonitoringTick()
{
  scoped_lock lock(m_mutex);

  if (m_alert) {
    m_alert->setProgress(m_pos);

    if (m_done || m_abort)
      m_alert->close();
  }
}

#endif

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
