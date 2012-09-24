// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "ui/timer.h"

#include "ui/manager.h"
#include "ui/message.h"
#include "ui/system.h"
#include "ui/widget.h"

#include <algorithm>
#include <list>

namespace ui {

typedef std::list<Timer*> Timers;

static Timers timers; // Registered timers

Timer::Timer(int interval, Widget* owner)
  : m_owner(owner ? owner: Manager::getDefault())
  , m_interval(interval)
  , m_lastTime(-1)
{
  ASSERT(m_owner != NULL);

  timers.push_back(this);
}

Timer::~Timer()
{
  Timers::iterator it = std::find(timers.begin(), timers.end(), this);
  ASSERT(it != timers.end());
  timers.erase(it);

  // Remove messages of this timer in the queue
  Manager::getDefault()->removeMessagesForTimer(this);
}

bool Timer::isRunning() const
{
  return (m_lastTime >= 0);
}

void Timer::start()
{
  m_lastTime = ji_clock;
}

void Timer::stop()
{
  m_lastTime = -1;
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
    int t = ji_clock;
    int count;

    for (Timers::iterator it=timers.begin(), end=timers.end(); it != end; ++it) {
      Timer* timer = *it;
      if (timer && timer->m_lastTime >= 0) {
        count = 0;
        while (t - timer->m_lastTime > timer->m_interval) {
          timer->m_lastTime += timer->m_interval;
          ++count;

          /* we spend too much time here */
          if (ji_clock - t > timer->m_interval) {
            timer->m_lastTime = ji_clock;
            break;
          }
        }

        if (count > 0) {
          ASSERT(timer->m_owner != NULL);

          Message* msg = jmessage_new(JM_TIMER);
          msg->timer.count = count;
          msg->timer.timer = timer;
          jmessage_add_dest(msg, timer->m_owner);
          Manager::getDefault()->enqueueMessage(msg);
        }
      }
    }
  }
}

void Timer::checkNoTimers()
{
  ASSERT(timers.empty());
}

} // namespace ui
