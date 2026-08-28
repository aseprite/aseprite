// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "app/ui/draggable_button_set.h"
#include "app/ui/button_set.h"
#include "app/ui/color_button_options.h"
#include "os/surface.h"
#include "ui/cursor.h"
#include "ui/cursor_type.h"
#include "ui/display.h"
#include "ui/manager.h"
#include "ui/message.h"

#include "base/log.h"
#include "ui/system.h"
#include <cstddef>

namespace app {

using namespace ui;

DraggableButtonSet::DraggableItem* DraggableButtonSet::m_draggedItem = nullptr;
DraggableButtonSet::DraggableItem* DraggableButtonSet::m_lastDragEnterItem = nullptr;

DraggableButtonSet::DraggableButtonSet(int columns, bool same_width_columns)
  : ButtonSet(columns, same_width_columns)
{
  setTriggerOnMouseUp(true);
}

bool DraggableButtonSet::DraggableItem::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {
    case ui::kMouseDownMessage: {
      bool consumed = Item::onProcessMessage(msg);
      if (!consumed) {
        if (auto* bs = dynamic_cast<DraggableButtonSet*>(buttonSet())) {
          if (bs->canStartDrag()) {
            DraggableButtonSet::m_draggedItem = this;
            DraggableButtonSet::m_lastDragEnterItem = nullptr;
            onDragItemStart(DraggableButtonSet::m_draggedItem);
            consumed = true;
          }
        }
      }
      return consumed;
    }

    case ui::kMouseMoveMessage: {
      if (!DraggableButtonSet::m_draggedItem) {
        return Item::onProcessMessage(msg);
      }

      auto* bs = dynamic_cast<DraggableButtonSet*>(buttonSet());
      if (hasCapture() && bs) {
        auto* mouseMsg = static_cast<ui::MouseMessage*>(msg);
        const gfx::Point screenPos = mouseMsg->display()->nativeWindow()->pointToScreen(
          mouseMsg->position());
        Manager* mgr = manager();
        Widget* pick = (mgr ? mgr->pickFromScreenPos(screenPos) : nullptr);
        if (pick && pick != this && DraggableButtonSet::m_lastDragEnterItem) {
          DraggableButtonSet::m_lastDragEnterItem->onDragItemLeave(
            DraggableButtonSet::m_draggedItem);
          DraggableButtonSet::m_lastDragEnterItem = nullptr;
        }

        auto* item = dynamic_cast<DraggableButtonSet::DraggableItem*>(pick);
        // Transfer mouse capture only to DraggableItems that are inside a DraggableButtonSet.
        if (item && item != this && dynamic_cast<DraggableButtonSet*>(item->buttonSet())) {
          releaseMouse();
          item->captureMouse();
          item->onDragItemEnter(DraggableButtonSet::m_draggedItem);
          DraggableButtonSet::m_lastDragEnterItem = item;
        }
        return true;
      }
      return false;
    }

    case ui::kMouseUpMessage: {
      if (!DraggableButtonSet::m_draggedItem) {
        return Item::onProcessMessage(msg);
      }

      if (hasCapture()) {
        auto* mouseMsg = static_cast<ui::MouseMessage*>(msg);
        const gfx::Point screenPos = mouseMsg->display()->nativeWindow()->pointToScreen(
          mouseMsg->position());
        Manager* mgr = manager();
        Widget* pick = (mgr ? mgr->pickFromScreenPos(screenPos) : nullptr);
        releaseMouse();
        invalidateItem();

        if (pick && pick == this) {
          onDropItem(DraggableButtonSet::m_draggedItem);
        }
      }

      onDragEnd();
      ButtonSet::resetOriginButtonset();
      DraggableButtonSet::m_draggedItem = nullptr;
      return true;
    }

    default: return Item::onProcessMessage(msg);
  }
}

void DraggableButtonSet::DraggableItem::onDragItemStart(DraggableItem* item)
{
#ifdef _DEBUG
  LOG(VERBOSE,
      "UI: [id=%s, type=%d]: onDragItemStart(), position: (%d, %d)\n",
      id().c_str(),
      type(),
      this->bounds().x,
      this->bounds().y);
#endif
}

void DraggableButtonSet::DraggableItem::onDragItemEnter(DraggableItem* item)
{
#ifdef _DEBUG
  LOG(VERBOSE,
      "UI: [id=%s, type=%d]: onDragItemEnter(), position: (%d, %d)\n",
      id().c_str(),
      type(),
      this->bounds().x,
      this->bounds().y);
#endif
  // When a dragged item enters this item, we must reorder the buttonset's items,
  // this implies modifying the children collection and the items positions.
  auto* bs = dynamic_cast<DraggableButtonSet*>(this->buttonSet());
  bs->moveItemTo(item, this);
}

void DraggableButtonSet::DraggableItem::onDragItemLeave(DraggableItem* item)
{
#ifdef _DEBUG
  LOG(VERBOSE,
      "UI: [id=%s, type=%d]: onDragItemLeave(), position: (%d, %d)\n",
      id().c_str(),
      type(),
      this->bounds().x,
      this->bounds().y);
#endif
}

void DraggableButtonSet::DraggableItem::onDropItem(DraggableItem* item)
{
#ifdef _DEBUG
  LOG(VERBOSE,
      "UI: [id=%s, type=%d]: onDropItem(), position: (%d, %d)\n",
      id().c_str(),
      type(),
      this->bounds().x,
      this->bounds().y);
#endif
}

void DraggableButtonSet::DraggableItem::onDragEnd()
{
#ifdef _DEBUG
  LOG(VERBOSE,
      "UI: [id=%s, type=%d]: onDragEnd(), position: (%d, %d)\n",
      id().c_str(),
      type(),
      this->bounds().x,
      this->bounds().y);
#endif
}

} // namespace app
