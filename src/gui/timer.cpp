// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "gui/timer.h"

#include "gui/manager.h"
#include "gui/message.h"
#include "gui/system.h"
#include "gui/widget.h"

#include <algorithm>
#include <vector>

namespace gui {

typedef std::vector<Timer*> Timers;

static Timers timers; // Registered timers

Timer::Timer(Widget* owner, int interval)
  : m_owner(owner)
  , m_interval(interval)
  , m_lastTime(-1)
{
  ASSERT_VALID_WIDGET(owner);

  timers.push_back(this);
}

Timer::~Timer()
{
  Timers::iterator it = std::find(timers.begin(), timers.end(), this);
  ASSERT(it != timers.end());
  timers.erase(it);

  // Remove messages of this timer in the queue
  jmanager_remove_messages_for_timer(this);
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

    for (int c=0; c<(int)timers.size(); ++c) {
      if (timers[c] && timers[c]->m_lastTime >= 0) {
        count = 0;
        while (t - timers[c]->m_lastTime > timers[c]->m_interval) {
          timers[c]->m_lastTime += timers[c]->m_interval;
          ++count;

          /* we spend too much time here */
          if (ji_clock - t > timers[c]->m_interval) {
            timers[c]->m_lastTime = ji_clock;
            break;
          }
        }

        if (count > 0) {
          Message* msg = jmessage_new(JM_TIMER);
          msg->timer.count = count;
          msg->timer.timer = timers[c];
          jmessage_add_dest(msg, timers[c]->m_owner);
          jmanager_enqueue_message(msg);
        }
      }
    }
  }
}

void Timer::checkNoTimers()
{
  ASSERT(timers.empty());
}

} // namespace gui
