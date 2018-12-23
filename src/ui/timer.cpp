// Aseprite UI Library
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/timer.h"

#include "base/time.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/system.h"
#include "ui/widget.h"

#include <algorithm>
#include <vector>

namespace ui {

typedef std::vector<Timer*> Timers;

static Timers timers; // Registered timers
static int running_timers = 0;

Timer::Timer(int interval, Widget* owner)
  : m_owner(owner ? owner: Manager::getDefault())
  , m_interval(interval)
  , m_running(false)
  , m_lastTick(0)
{
  ASSERT(m_owner != nullptr);
  assert_ui_thread();

  timers.push_back(this);
}

Timer::~Timer()
{
  assert_ui_thread();

  auto it = std::find(timers.begin(), timers.end(), this);
  ASSERT(it != timers.end());
  if (it != timers.end())
    timers.erase(it);

  // Stop the timer and remove it from the message queue.
  stop();
}

void Timer::start()
{
  assert_ui_thread();

  m_lastTick = base::current_tick();
  if (!m_running) {
    m_running = true;
    ++running_timers;
  }
}

void Timer::stop()
{
  assert_ui_thread();

  if (m_running) {
    m_running = false;
    --running_timers;

    // Remove messages of this timer in the queue. The expected behavior
    // is that when we stop a timer, we'll not receive more messages
    // about it (even if there are enqueued messages waiting in the
    // message queue).
    Manager::getDefault()->removeMessagesForTimer(this);
  }
}

void Timer::tick()
{
  assert_ui_thread();

  onTick();
}

void Timer::setInterval(int interval)
{
  m_interval = interval;
}

void Timer::onTick()
{
  // Fire Tick signal.
  Tick();
}

void Timer::pollTimers()
{
  assert_ui_thread();

  // Generate messages for timers
  if (running_timers != 0) {
    ASSERT(!timers.empty());
    base::tick_t t = base::current_tick();

    for (auto timer : timers) {
      if (timer && timer->isRunning()) {
        int64_t count = ((t - timer->m_lastTick) / timer->m_interval);
        if (count > 0) {
          timer->m_lastTick += count * timer->m_interval;

          ASSERT(timer->m_owner != nullptr);

          Message* msg = new TimerMessage(count, timer);
          msg->setRecipient(timer->m_owner);
          Manager::getDefault()->enqueueMessage(msg);
        }
      }
    }
  }
}

bool Timer::haveTimers()
{
  return !timers.empty();
}

bool Timer::haveRunningTimers()
{
  return (running_timers != 0);
}

} // namespace ui
