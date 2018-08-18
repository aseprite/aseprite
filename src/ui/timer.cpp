// Aseprite UI Library
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/timer.h"

#include "base/time.h"
#include "obs/safe_list.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/widget.h"

namespace ui {

typedef obs::safe_list<Timer> Timers;

static Timers timers; // Registered timers

Timer::Timer(int interval, Widget* owner)
  : m_owner(owner ? owner: Manager::getDefault())
  , m_interval(interval)
  , m_running(false)
  , m_lastTick(0)
{
  ASSERT(m_owner != nullptr);

  timers.push_back(this);
}

Timer::~Timer()
{
  timers.erase(this);

  // Stop the timer and remove it from the message queue.
  stop();
}

void Timer::start()
{
  m_lastTick = base::current_tick();
  m_running = true;
}

void Timer::stop()
{
  m_running = false;

  // Remove messages of this timer in the queue. The expected behavior
  // is that when we stop a timer, we'll not receive more messages
  // about it (even if there are enqueued messages waiting in the
  // message queue).
  Manager::getDefault()->removeMessagesForTimer(this);
}

void Timer::tick()
{
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
  // Generate messages for timers
  if (!timers.empty()) {
    base::tick_t t = base::current_tick();

    for (auto timer : timers) {
      if (timer && timer->isRunning()) {
        int64_t count = ((t - timer->m_lastTick) / timer->m_interval);
        if (count > 0) {
          timer->m_lastTick += count * timer->m_interval;

          ASSERT(timer->m_owner != nullptr);

          Message* msg = new TimerMessage(count, timer);
          msg->addRecipient(timer->m_owner);
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
  if (!timers.empty()) {
    for (auto timer : timers) {
      if (timer && timer->isRunning()) {
        return true;
      }
    }
  }
  return false;
}

} // namespace ui
