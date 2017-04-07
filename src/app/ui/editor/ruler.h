// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_RULER_H_INCLUDED
#define APP_UI_EDITOR_RULER_H_INCLUDED
#pragma once

namespace app {

  // A ruler inside the editor. It is used by SelectBoxState to show
  // rulers that can be dragged by the user.
  class Ruler {
  public:
    enum Orientation { Horizontal, Vertical };

    Ruler()
      : m_orientation(Horizontal)
      , m_position(0) {
    }

    Ruler(Orientation orientation, int position)
      : m_orientation(orientation)
      , m_position(position) {
    }

    Orientation orientation() const { return m_orientation; }
    int position() const { return m_position; }

    void setPosition(int position) {
      m_position = position;
    }

  private:
    Orientation m_orientation;
    int m_position;
  };

} // namespace app

#endif // APP_UI_EDITOR_RULER_H_INCLUDED
