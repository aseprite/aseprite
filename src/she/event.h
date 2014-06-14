// SHE library
// Copyright (C) 2012-2014  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef SHE_EVENT_H_INCLUDED
#define SHE_EVENT_H_INCLUDED
#pragma once

#include "gfx/point.h"

#include <string>
#include <vector>

namespace she {

  class Event {
  public:
    enum Type {
      None,
      DropFiles,
      MouseEnter,
      MouseLeave,
      MouseMove,
      MouseDown,
      MouseUp,
      MouseWheel,
      MouseDoubleClick,
    };

    enum MouseButton {
      NoneButton,
      LeftButton,
      RightButton,
      MiddleButton
    };

    typedef std::vector<std::string> Files;

    Event() : m_type(None) { }

    Type type() const { return m_type; }
    const Files& files() const { return m_files; }
    gfx::Point position() const { return m_position; }
    gfx::Point wheelDelta() const { return m_wheelDelta; }
    MouseButton button() const { return m_button; }

    void setType(Type type) { m_type = type; }
    void setFiles(const Files& files) { m_files = files; }
    void setPosition(const gfx::Point& pos) { m_position = pos; }
    void setWheelDelta(const gfx::Point& delta) { m_wheelDelta = delta; }
    void setButton(MouseButton button) { m_button = button; }

  private:
    Type m_type;
    Files m_files;
    gfx::Point m_position;
    gfx::Point m_wheelDelta;
    MouseButton m_button;
  };

} // namespace she

#endif
