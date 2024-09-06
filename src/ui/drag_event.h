// Aseprite UI Library
// Copyright (C) 2024  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_DRAG_EVENT_H_INCLUDED
#define UI_DRAG_EVENT_H_INCLUDED
#pragma once

#include "base/paths.h"
#include "os/dnd.h"
#include "os/surface.h"
#include "ui/event.h"
#include "ui/widget.h"

namespace ui {

  class DragEvent : public Event {
  public:
    DragEvent(Component* source, ui::Widget* target, os::DragEvent& ev)
      : Event(source)
      , m_position(ev.position() - target->bounds().origin())
      , m_ev(ev) {}

    bool handled() const { return m_handled; }
    void handled(bool value) { m_handled = value; }

    // Operations allowed by the source of the drag & drop operation. Can be a
    // bitwise combination of values.
    os::DropOperation allowedOperations() const { return m_ev.supportedOperations(); }

    // Operation supported by the target of the drag & drop operation. Cannot
    // be a bitwise combination of values.
    os::DropOperation supportsOperation() const { return m_ev.dropResult(); }
    // Set the operation supported by the target of the drag & drop operation.
    // Cannot be a bitwise combination of values.
    void supportsOperation(os::DropOperation operation) { m_ev.dropResult(operation); }

    const gfx::Point& position() const { return m_position; }

    bool hasPaths() const {
      return m_ev.dataProvider()->contains(os::DragDataItemType::Paths);
    }

    bool hasImage() const {
      return m_ev.dataProvider()->contains(os::DragDataItemType::Image);
    }

    base::paths getPaths() const {
      return m_ev.dataProvider()->getPaths();
    }

    os::SurfaceRef getImage() const {
      return m_ev.dataProvider()->getImage();
    }

  private:
    bool m_handled = false;
    gfx::Point m_position;
    os::DragEvent& m_ev;
  };

} // namespace ui

#endif  // UI_DRAG_EVENT_H_INCLUDED
