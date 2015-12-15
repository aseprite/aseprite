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

AppBrushes::slot_id AppBrushes::addCustomBrush(const BrushRef& brush)
{
  // Use an empty slot
  for (size_t i=0; i<m_slots.size(); ++i) {
    if (!m_slots[i].locked() ||
        !m_slots[i].brush()) {
      m_slots[i].setBrush(brush);
      return i+1;
    }
  }

  m_slots.push_back(BrushSlot(brush));
  ItemsChange();
  return (int)m_slots.size(); // Returns the slot
}

void AppBrushes::removeCustomBrush(slot_id slot)
{
  --slot;
  if (slot >= 0 && slot < (int)m_slots.size()) {
    m_slots[slot].setBrush(BrushRef(nullptr));

    // Erase empty trailing slots
    while (!m_slots.empty() &&
           !m_slots[m_slots.size()-1].brush())
      m_slots.erase(--m_slots.end());

    ItemsChange();
  }
}

void AppBrushes::removeAllCustomBrushes()
{
  while (!m_slots.empty())
    m_slots.erase(--m_slots.end());

  ItemsChange();
}

bool AppBrushes::hasCustomBrush(slot_id slot) const
{
  --slot;
  return (slot >= 0 && slot < (int)m_slots.size() &&
          m_slots[slot].brush());
}

BrushRef AppBrushes::getCustomBrush(slot_id slot) const
{
  --slot;
  if (slot >= 0 && slot < (int)m_slots.size())
    return m_slots[slot].brush();
  else
    return BrushRef();
}

void AppBrushes::setCustomBrush(slot_id slot, const doc::BrushRef& brush)
{
  --slot;
  if (slot >= 0 && slot < (int)m_slots.size()) {
    m_slots[slot].setBrush(brush);
    ItemsChange();
  }
}

Brushes AppBrushes::getCustomBrushes()
{
  Brushes brushes;
  for (const auto& slot : m_slots)
    brushes.push_back(slot.brush());
  return brushes;
}

void AppBrushes::lockCustomBrush(slot_id slot)
{
  --slot;
  if (slot >= 0 && slot < (int)m_slots.size() &&
      m_slots[slot].brush()) {
    m_slots[slot].setLocked(true);
  }
}

void AppBrushes::unlockCustomBrush(slot_id slot)
{
  --slot;
  if (slot >= 0 && slot < (int)m_slots.size() &&
      m_slots[slot].brush()) {
    m_slots[slot].setLocked(false);
  }
}

bool AppBrushes::isCustomBrushLocked(slot_id slot) const
{
  --slot;
  if (slot >= 0 && slot < (int)m_slots.size() &&
      m_slots[slot].brush()) {
    return m_slots[slot].locked();
  }
  else
    return false;
}

} // namespace app
