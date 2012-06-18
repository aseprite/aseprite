// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_TIMER_H_INCLUDED
#define GUI_TIMER_H_INCLUDED

#include "base/disable_copying.h"
#include "base/signal.h"

namespace ui {

  class Widget;

  class Timer
  {
  public:
    Timer(Widget* owner, int interval);
    virtual ~Timer();

    int getInterval() const;
    void setInterval(int interval);

    bool isRunning() const;

    void start();
    void stop();

    void tick();

    Signal0<void> Tick;

    static void pollTimers();
    static void checkNoTimers();

  protected:
    virtual void onTick();

  public:
    Widget* m_owner;
    int m_interval;
    int m_lastTime;

    DISABLE_COPYING(Timer);
  };

} // namespace ui

#endif
