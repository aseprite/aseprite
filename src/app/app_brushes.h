// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_APP_BRUSHES_H_INCLUDED
#define APP_APP_BRUSHES_H_INCLUDED
#pragma once

#include "base/signal.h"
#include "doc/brush.h"
#include "doc/brushes.h"

#include <vector>

namespace app {

  class AppBrushes {
  public:
    // Number of slot (a range from 1 to AppBrushes::size() inclusive)
    typedef int slot_id;

    AppBrushes();

    // Adds a new brush and returns the slot number where the brush
    // is now available.
    slot_id addCustomBrush(const doc::BrushRef& brush);
    void removeCustomBrush(slot_id slot);
    void removeAllCustomBrushes();
    bool hasCustomBrush(slot_id slot) const;
    const doc::Brushes& getStandardBrushes() { return m_standard; }
    doc::BrushRef getCustomBrush(slot_id slot) const;
    void setCustomBrush(slot_id slot, const doc::BrushRef& brush);
    doc::Brushes getCustomBrushes();

    void lockCustomBrush(slot_id slot);
    void unlockCustomBrush(slot_id slot);
    bool isCustomBrushLocked(slot_id slot) const;

    base::Signal0<void> ItemsChange;

  private:
    // Custom brush slot
    class BrushSlot {
    public:
      BrushSlot(const doc::BrushRef& brush)
        : m_locked(false)
        , m_brush(brush) {
      }

      // True if this is a standard brush.
      bool standard() const { return m_standard; }

      // True if the user locked the brush using the shortcut key to
      // access it.
      bool locked() const { return m_locked; }
      void setLocked(bool locked) { m_locked = locked; }

      // Can be null if the user deletes the brush.
      doc::BrushRef brush() const { return m_brush; }
      void setBrush(const doc::BrushRef& brush) { m_brush = brush; }

    private:
      bool m_standard;
      bool m_locked;
      doc::BrushRef m_brush;
    };

    typedef std::vector<BrushSlot> BrushSlots;

    doc::Brushes m_standard;
    BrushSlots m_slots;
  };

} // namespace app

#endif
