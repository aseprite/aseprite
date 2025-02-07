// Aseprite
// Copyright (C) 2019-2024  Igara Studio S.A.
// Copyright (C) 2017-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/editor/moving_selection_state.h"

#include "app/cmd/set_mask_position.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/console.h"
#include "app/context_access.h"
#include "app/tx.h"
#include "app/ui/editor/editor.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "doc/mask.h"
#include "fmt/format.h"
#include "ui/message.h"

namespace app {

using namespace ui;

MovingSelectionState::MovingSelectionState(Editor* editor, MouseMessage* msg)
  : m_editor(editor)
  , m_cursorStart(editor->screenToEditor(msg->position()))
  , m_selOrigin(editor->document()->mask()->bounds().origin())
  , m_selectionCanceled(false)
{
  editor->captureMouse();

  // Hook BeforeCommandExecution signal so we know if the user wants
  // to execute other command, so we can unfreeze the document mask.
  m_ctxConn = UIContext::instance()->BeforeCommandExecution.connect(
    &MovingSelectionState::onBeforeCommandExecution,
    this);
}

void MovingSelectionState::onBeforeCommandExecution(CommandExecutionEvent& ev)
{
  if (ev.command()->id() == CommandId::Cancel()) {
    // We cancel the cancel command to avoid calling the inputChain()->cancel(), which
    // deselects the selection.
    ev.cancel();
    m_selectionCanceled = true;
  }
  m_editor->backToPreviousState();
  m_editor->releaseMouse();
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

  ASSERT(mask->isFrozen());

  // Restore the mask to the original state so we can transform it
  // with the a undoable transaction.
  mask->setOrigin(m_selOrigin.x, m_selOrigin.y);
  mask->unfreeze();

  if (m_selectionCanceled) {
    doc->resetTransformation();
    doc->generateMaskBoundaries();
  }
  else {
    try {
      ContextWriter writer(UIContext::instance(), 1000);
      Tx tx(writer, "Move Selection Edges", DoesntModifyDocument);
      tx(new cmd::SetMaskPosition(doc, newOrigin));
      tx.commit();
    }
    catch (const base::Exception& e) {
      Console::showException(e);
    }
    doc->resetTransformation();
  }
  doc->notifyGeneralUpdate();
  return StandbyState::onLeaveState(editor, newState);
}

void MovingSelectionState::onBeforePopState(Editor* editor)
{
  m_ctxConn.disconnect();
  StandbyState::onBeforePopState(editor);
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

  ASSERT(editor->document()->mask()->isFrozen());

  if (oldMaskOrigin != newMaskOrigin) {
    editor->document()->mask()->setOrigin(newMaskOrigin.x, newMaskOrigin.y);

    if (editor->document()->hasMaskBoundaries()) {
      MaskBoundaries& boundaries = editor->document()->maskBoundaries();
      const gfx::Point boundariesDelta = newMaskOrigin - oldMaskOrigin;
      boundaries.offset(boundariesDelta.x, boundariesDelta.y);
    }
    else {
      ASSERT(false);
    }

    editor->invalidate();
  }

  // Use StandbyState implementation
  return StandbyState::onMouseMove(editor, msg);
}

bool MovingSelectionState::onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos)
{
  auto theme = skin::SkinTheme::get(editor);
  editor->showMouseCursor(kCustomCursor, theme->cursors.moveSelection());
  return true;
}

bool MovingSelectionState::onUpdateStatusBar(Editor* editor)
{
  const gfx::Rect bounds = editor->document()->mask()->bounds();

  StatusBar::instance()->setStatusText(100,
                                       fmt::format(":pos: {} {}"
                                                   " :size: {} {}"
                                                   " :delta: {} {}",
                                                   bounds.x,
                                                   bounds.y,
                                                   bounds.w,
                                                   bounds.h,
                                                   m_delta.x,
                                                   m_delta.y));

  return true;
}

} // namespace app
