// Aseprite UI Library
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_TIMER_H_INCLUDED
#define UI_TIMER_H_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "base/time.h"
#include "obs/signal.h"

namespace ui {

  class Widget;

  class Timer {
  public:
    Timer(int interval, Widget* owner = NULL);
    virtual ~Timer();

    int interval() const { return m_interval; }
    void setInterval(int interval);

    bool isRunning() const {
      return m_running;
    }

    void start();
    void stop();

    void tick();

    obs::signal<void()> Tick;

    static void pollTimers();
    static bool haveTimers();
    static bool haveRunningTimers();

  protected:
    virtual void onTick();

  public:
    Widget* m_owner;
    int m_interval;
    bool m_running;
    base::tick_t m_lastTick;

    DISABLE_COPYING(Timer);
  };

} // namespace ui

#endif
