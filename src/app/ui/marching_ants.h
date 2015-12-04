// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_MARCHING_ANTS_H_INCLUDED
#define APP_UI_MARCHING_ANTS_H_INCLUDED
#pragma once

#include "base/connection.h"
#include "ui/timer.h"

#include <cmath>

namespace app {

  class MarchingAnts {
  public:
    MarchingAnts()
      : m_timer(100)
      , m_offset(0)
    {
      m_scopedConn = m_timer.Tick.connect(&MarchingAnts::onTick, this);
    }

    ~MarchingAnts() {
      m_timer.stop();
    }

  protected:
    virtual void onDrawMarchingAnts() = 0;

    int getMarchingAntsOffset() const {
      return m_offset;
    }

    bool isMarchingAntsRunning() const {
      return m_timer.isRunning();
    }

    void startMarchingAnts() {
      m_timer.start();
    }

    void stopMarchingAnts() {
      m_timer.stop();
    }

  private:
    void onTick() {
      m_offset = ((m_offset+1) % 8);
      onDrawMarchingAnts();
    }

    ui::Timer m_timer;
    int m_offset;
    base::ScopedConnection m_scopedConn;
  };

} // namespace app

#endif
