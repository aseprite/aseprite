// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef UI_TIMER_H_INCLUDED
#define UI_TIMER_H_INCLUDED

#include "base/disable_copying.h"
#include "base/signal.h"

namespace ui {

  class Widget;

  class Timer
  {
  public:
    Timer(int interval, Widget* owner = NULL);
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
