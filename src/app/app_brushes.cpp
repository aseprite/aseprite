// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app_brushes.h"

namespace app {

using namespace doc;

AppBrushes::AppBrushes()
{
  m_standard.push_back(BrushRef(new Brush(kCircleBrushType, 7, 0)));
  m_standard.push_back(BrushRef(new Brush(kSquareBrushType, 7, 0)));
  m_standard.push_back(BrushRef(new Brush(kLineBrushType, 7, 44)));
}

AppBrushes::slot_id AppBrushes::addBrushSlot(const BrushSlot& brush)
{
  // Use an empty slot
  for (size_t i=0; i<m_slots.size(); ++i) {
    if (!m_slots[i].locked() || m_slots[i].isEmpty()) {
      m_slots[i] = brush;
      return i+1;
    }
  }

  m_slots.push_back(brush);
  ItemsChange();
  return slot_id(m_slots.size()); // Returns the slot
}

void AppBrushes::removeBrushSlot(slot_id slot)
{
  --slot;
  if (slot >= 0 && slot < (int)m_slots.size()) {
    m_slots[slot] = BrushSlot();

    // Erase empty trailing slots
    while (!m_slots.empty() &&
           m_slots[m_slots.size()-1].isEmpty())
      m_slots.erase(--m_slots.end());

    ItemsChange();
  }
}

void AppBrushes::removeAllBrushSlots()
{
  while (!m_slots.empty())
    m_slots.erase(--m_slots.end());

  ItemsChange();
}

bool AppBrushes::hasBrushSlot(slot_id slot) const
{
  --slot;
  return (slot >= 0 && slot < (int)m_slots.size() &&
          !m_slots[slot].isEmpty());
}

BrushSlot AppBrushes::getBrushSlot(slot_id slot) const
{
  --slot;
  if (slot >= 0 && slot < (int)m_slots.size())
    return m_slots[slot];
  else
    return BrushSlot();
}

void AppBrushes::setBrushSlot(slot_id slot, const BrushSlot& brush)
{
  --slot;
  if (slot >= 0 && slot < (int)m_slots.size()) {
    m_slots[slot] = brush;
    ItemsChange();
  }
}

void AppBrushes::lockBrushSlot(slot_id slot)
{
  --slot;
  if (slot >= 0 && slot < (int)m_slots.size() &&
      !m_slots[slot].isEmpty()) {
    m_slots[slot].setLocked(true);
  }
}

void AppBrushes::unlockBrushSlot(slot_id slot)
{
  --slot;
  if (slot >= 0 && slot < (int)m_slots.size() &&
      !m_slots[slot].isEmpty()) {
    m_slots[slot].setLocked(false);
  }
}

bool AppBrushes::isBrushSlotLocked(slot_id slot) const
{
  --slot;
  if (slot >= 0 && slot < (int)m_slots.size() &&
      !m_slots[slot].isEmpty()) {
    return m_slots[slot].locked();
  }
  else
    return false;
}

} // namespace app
