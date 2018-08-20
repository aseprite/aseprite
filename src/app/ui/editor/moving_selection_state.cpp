// Aseprite
// Copyright (C) 2017-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/moving_selection_state.h"

#include "app/cmd/set_mask_position.h"
#include "app/context_access.h"
#include "app/tx.h"
#include "app/ui/editor/editor.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "doc/mask.h"
#include "ui/message.h"

namespace app {

using namespace ui;

MovingSelectionState::MovingSelectionState(Editor* editor, MouseMessage* msg)
  : m_cursorStart(editor->screenToEditor(msg->position()))
  , m_selOrigin(editor->document()->mask()->bounds().origin())
{
  editor->captureMouse();
}

MovingSelectionState::~MovingSelectionState()
{
}

void MovingSelectionState::onEnterState(Editor* editor)
{
  StandbyState::onEnterState(editor);
  editor->document()->mask()->freeze();
}

EditorState::LeaveAction MovingSelectionState::onLeaveState(Editor* editor, EditorState* newState)
{
  Doc* doc = editor->document();
  Mask* mask = doc->mask();
  gfx::Point newOrigin = mask->bounds().origin();

  // Restore the mask to the original state so we can transform it
  // with the a undoable transaction.
  mask->setOrigin(m_selOrigin.x,
                  m_selOrigin.y);
  mask->unfreeze();

  {
    ContextWriter writer(UIContext::instance(), 1000);
    Tx tx(writer.context(), "Move Selection Edges", DoesntModifyDocument);
    tx(new cmd::SetMaskPosition(doc, newOrigin));
    tx.commit();
  }

  doc->resetTransformation();
  doc->notifyGeneralUpdate();
  return StandbyState::onLeaveState(editor, newState);
}

bool MovingSelectionState::onMouseDown(Editor* editor, MouseMessage* msg)
{
  // Do nothing
  return true;
}

bool MovingSelectionState::onMouseUp(Editor* editor, MouseMessage* msg)
{
  editor->backToPreviousState();
  editor->releaseMouse();
  return true;
}

bool MovingSelectionState::onMouseMove(Editor* editor, MouseMessage* msg)
{
  const gfx::Point mousePos = editor->autoScroll(msg, AutoScroll::MouseDir);
  const gfx::Point newCursorPos = editor->screenToEditor(mousePos);
  m_delta = newCursorPos - m_cursorStart;
  const gfx::Point newMaskOrigin = m_selOrigin + m_delta;
  const gfx::Point oldMaskOrigin = editor->document()->mask()->bounds().origin();

  if (oldMaskOrigin != newMaskOrigin) {
    editor->document()->mask()->setOrigin(newMaskOrigin.x,
                                          newMaskOrigin.y);

    MaskBoundaries* boundaries =
      const_cast<MaskBoundaries*>(editor->document()->getMaskBoundaries());
    const gfx::Point boundariesDelta = newMaskOrigin - oldMaskOrigin;
    boundaries->offset(boundariesDelta.x,
                       boundariesDelta.y);

    editor->invalidate();
  }

  // Use StandbyState implementation
  return StandbyState::onMouseMove(editor, msg);
}

bool MovingSelectionState::onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos)
{
  editor->showMouseCursor(
    kCustomCursor, skin::SkinTheme::instance()->cursors.moveSelection());
  return true;
}

bool MovingSelectionState::onUpdateStatusBar(Editor* editor)
{
  const gfx::Rect bounds = editor->document()->mask()->bounds();

  StatusBar::instance()->setStatusText
    (100, ":pos: %d %d :size: %3d %3d :offset: %d %d",
     bounds.x, bounds.y,
     bounds.w, bounds.h,
     m_delta.x, m_delta.y);

  return true;
}

} // namespace app
