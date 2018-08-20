// Aseprite
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

#include <cmath>

namespace app {

using namespace ui;

MovingSliceState::MovingSliceState(Editor* editor, MouseMessage* msg,
                                   const EditorHit& hit)
  : m_hit(hit)
{
  m_mouseStart = editor->screenToEditor(msg->position());

  auto keyPtr = m_hit.slice()->getByFrame(editor->frame());
  ASSERT(keyPtr);
  if (keyPtr)
    m_keyStart = m_key = *keyPtr;

  editor->captureMouse();
}

bool MovingSliceState::onMouseUp(Editor* editor, MouseMessage* msg)
{
  {
    ContextWriter writer(UIContext::instance(), 1000);
    Tx tx(writer.context(), "Slice Movement", ModifyDocument);

    doc::SliceKey newKey = m_key;
    m_hit.slice()->insert(editor->frame(), m_keyStart);

    tx(new cmd::SetSliceKey(m_hit.slice(),
                            editor->frame(),
                            newKey));
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

  m_key = m_keyStart;
  gfx::Rect rc =
    (m_hit.type() == EditorHit::SliceCenter ? m_key.center():
                                              m_key.bounds());

  // Move slice
  if (m_hit.border() == (CENTER | MIDDLE)) {
    rc.x += delta.x;
    rc.y += delta.y;
  }
  else {
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

  if (m_hit.type() == EditorHit::SliceCenter)
    m_key.setCenter(rc);
  else
    m_key.setBounds(rc);

  // Update the slice key
  m_hit.slice()->insert(editor->frame(), m_key);

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

} // namespace app
