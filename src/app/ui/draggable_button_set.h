// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_DRAGGABLE_BUTTON_SET_H_INCLUDED
#define APP_UI_DRAGGABLE_BUTTON_SET_H_INCLUDED
#pragma once

#include "app/ui/button_set.h"
#include "ui/cursor.h"

#include <memory>

namespace app {

class DraggableButtonSet : public ButtonSet {
public:
  class DraggableItem : public Item {
  public:
    DraggableItem() {}

  protected:
    bool onProcessMessage(ui::Message* msg);

    virtual void onDragItemStart(DraggableItem* item);
    virtual void onDragItemEnter(DraggableItem* item);
    virtual void onDragItemLeave(DraggableItem* item);
    virtual void onDropItem(DraggableItem* item);
    virtual void onDragEnd();
  };

  DraggableButtonSet(int columns, bool same_width_columns = false);

protected:
  virtual bool canStartDrag() { return this == ButtonSet::originButtonset(); };

private:
  // Dragged buttonset's item. It is static because we want to support drag &
  // drop between different draggable buttonsets.
  static DraggableItem* m_draggedItem;

  // Item where the drag entered the last time.
  static DraggableItem* m_lastDragEnterItem;
};

} // namespace app

#endif
