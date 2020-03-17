// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2017-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/moving_slice_state.h"

#include "app/cmd/set_slice_key.h"
#include "app/context_access.h"
#include "app/tx.h"
#include "app/ui/editor/editor.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "doc/slice.h"
#include "ui/message.h"

#include <algorithm>
#include <cmath>

namespace app {

using namespace ui;

MovingSliceState::MovingSliceState(Editor* editor,
                                   MouseMessage* msg,
                                   const EditorHit& hit,
                                   const doc::SelectedObjects& selectedSlices)
  : m_frame(editor->frame())
  , m_hit(hit)
  , m_items(std::max<std::size_t>(1, selectedSlices.size()))
{
  m_mouseStart = editor->screenToEditor(msg->position());

  if (selectedSlices.empty()) {
    m_items[0] = getItemForSlice(m_hit.slice());
  }
  else {
    int i = 0;
    for (Slice* slice : selectedSlices.iterateAs<Slice>()) {
      ASSERT(slice);
      m_items[i++] = getItemForSlice(slice);
    }
  }

  editor->captureMouse();
}

bool MovingSliceState::onMouseUp(Editor* editor, MouseMessage* msg)
{
  {
    ContextWriter writer(UIContext::instance(), 1000);
    Tx tx(writer.context(), "Slice Movement", ModifyDocument);

    for (const auto& item : m_items) {
      item.slice->insert(m_frame, item.oldKey);
      tx(new cmd::SetSliceKey(item.slice, m_frame, item.newKey));
    }

    tx.commit();
  }

  editor->backToPreviousState();
  editor->releaseMouse();
  return true;
}

bool MovingSliceState::onMouseMove(Editor* editor, MouseMessage* msg)
{
  gfx::Point newCursorPos = editor->screenToEditor(msg->position());
  gfx::Point delta = newCursorPos - m_mouseStart;
  gfx::Rect totalBounds = selectedSlicesBounds();

  ASSERT(totalBounds.w > 0);
  ASSERT(totalBounds.h > 0);

  for (auto& item : m_items) {
    auto& key = item.newKey;
    key = item.oldKey;

    gfx::Rect rc =
      (m_hit.type() == EditorHit::SliceCenter ? key.center():
                                                key.bounds());

    // Move slice
    if (m_hit.border() == (CENTER | MIDDLE)) {
      rc.x += delta.x;
      rc.y += delta.y;
    }
    // Move/resize 9-slices center
    else if (m_hit.type() == EditorHit::SliceCenter) {
      if (m_hit.border() & LEFT) {
        rc.x += delta.x;
        rc.w -= delta.x;
        if (rc.w < 1) {
          rc.x += rc.w-1;
          rc.w = 1;
        }
      }
      if (m_hit.border() & TOP) {
        rc.y += delta.y;
        rc.h -= delta.y;
        if (rc.h < 1) {
          rc.y += rc.h-1;
          rc.h = 1;
        }
      }
      if (m_hit.border() & RIGHT) {
        rc.w += delta.x;
        if (rc.w < 1)
          rc.w = 1;
      }
      if (m_hit.border() & BOTTOM) {
        rc.h += delta.y;
        if (rc.h < 1)
          rc.h = 1;
      }
    }
    // Move/resize bounds
    else {
      if (m_hit.border() & LEFT) {
        rc.x += delta.x * (totalBounds.x2() - rc.x) / totalBounds.w;
        rc.w -= delta.x * rc.w / totalBounds.w;
        if (rc.w < 1) {
          rc.x += rc.w-1;
          rc.w = 1;
        }
      }
      if (m_hit.border() & TOP) {
        rc.y += delta.y * (totalBounds.y2() - rc.y) / totalBounds.h;
        rc.h -= delta.y * rc.h / totalBounds.h;
        if (rc.h < 1) {
          rc.y += rc.h-1;
          rc.h = 1;
        }
      }
      if (m_hit.border() & RIGHT) {
        rc.x += delta.x * (rc.x - totalBounds.x) / totalBounds.w;
        rc.w += delta.x * rc.w / totalBounds.w;
        if (rc.w < 1)
          rc.w = 1;
      }
      if (m_hit.border() & BOTTOM) {
        rc.y += delta.y * (rc.y - totalBounds.y) / totalBounds.h;
        rc.h += delta.y * rc.h / totalBounds.h;
        if (rc.h < 1)
          rc.h = 1;
      }
    }

    if (m_hit.type() == EditorHit::SliceCenter)
      key.setCenter(rc);
    else
      key.setBounds(rc);

    // Update the slice key
    item.slice->insert(m_frame, key);
  }

  // Redraw the editor.
  editor->invalidate();

  // Use StandbyState implementation
  return StandbyState::onMouseMove(editor, msg);
}

bool MovingSliceState::onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos)
{
  switch (m_hit.border()) {
    case TOP | LEFT:
      editor->showMouseCursor(kSizeNWCursor);
      break;
    case TOP:
      editor->showMouseCursor(kSizeNCursor);
      break;
    case TOP | RIGHT:
      editor->showMouseCursor(kSizeNECursor);
      break;
    case LEFT:
      editor->showMouseCursor(kSizeWCursor);
      break;
    case RIGHT:
      editor->showMouseCursor(kSizeECursor);
      break;
    case BOTTOM | LEFT:
      editor->showMouseCursor(kSizeSWCursor);
      break;
    case BOTTOM:
      editor->showMouseCursor(kSizeSCursor);
      break;
    case BOTTOM | RIGHT:
      editor->showMouseCursor(kSizeSECursor);
      break;
  }
  return true;
}

MovingSliceState::Item MovingSliceState::getItemForSlice(doc::Slice* slice)
{
  Item item;
  item.slice = slice;

  auto keyPtr = slice->getByFrame(m_frame);
  ASSERT(keyPtr);
  if (keyPtr)
    item.oldKey = item.newKey = *keyPtr;

  return item;
}

gfx::Rect MovingSliceState::selectedSlicesBounds() const
{
  gfx::Rect bounds;
  for (auto& item : m_items)
    bounds |= item.oldKey.bounds();
  return bounds;
}

} // namespace app
