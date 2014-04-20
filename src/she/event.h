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

    int type() const { return m_type; }
    const Files& files() const { return m_files; }
    gfx::Point position() const { return m_position; }
    MouseButton button() const { return m_button; }
    int delta() const { return m_delta; }

    void setType(Type type) { m_type = type; }
    void setFiles(const Files& files) { m_files = files; }
    void setPosition(const gfx::Point& pos) { m_position = pos; }
    void setButton(MouseButton button) { m_button = button; }
    void setDelta(int delta) { m_delta = delta; }

  private:
    Type m_type;
    Files m_files;
    gfx::Point m_position;
    MouseButton m_button;
    int m_delta;
  };

} // namespace she

#endif
