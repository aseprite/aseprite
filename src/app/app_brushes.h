// Aseprite
// Copyright (C) 2021  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_APP_BRUSHES_H_INCLUDED
#define APP_APP_BRUSHES_H_INCLUDED
#pragma once

#include "app/brush_pattern_slot.h"
#include "app/brush_slot.h"
#include "doc/brush_patterns.h"
#include "doc/brushes.h"
#include "obs/signal.h"

#include <string>
#include <vector>

namespace app {

class AppBrushes {
public:
  // Number of slot (a range from 1 to AppBrushes::size() inclusive)
  typedef int slot_id;
  typedef std::vector<BrushSlot> BrushSlots;
  typedef std::vector<BrushPatternSlot> BrushPatternSlots;

  AppBrushes();
  ~AppBrushes();

  void reset();

  // Adds a new brush and returns the slot number where the brush
  // is now available.
  slot_id addBrushSlot(const BrushSlot& brush);
  void removeBrushSlot(slot_id slot);
  void removeAllBrushSlots();
  bool hasBrushSlot(slot_id slot) const;
  const doc::Brushes& getStandardBrushes() { return m_standard; }
  const doc::BrushPatterns& getStandardPatterns() { return m_stdPatterns; }
  BrushSlot getBrushSlot(slot_id slot) const;
  void setBrushSlot(slot_id slot, const BrushSlot& brush);
  const BrushSlots& getBrushSlots() const { return m_slots; }
  const BrushPatternSlots& getPatternSlots() const { return m_patternSlots; }

  void lockBrushSlot(slot_id slot);
  void unlockBrushSlot(slot_id slot);
  bool isBrushSlotLocked(slot_id slot) const;

  obs::signal<void()> ItemsChange;

  static std::string userBrushesFilename();

private:
  void init();
  void load(const std::string& filename);
  void save(const std::string& filename) const;

  doc::Brushes m_standard;
  doc::BrushPatterns m_stdPatterns;
  BrushPatternSlots m_patternSlots;
  BrushSlots m_slots;
  std::string m_userBrushesFilename;
};

} // namespace app

#endif
