// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/document_undo.h"

#include "app/app.h"
#include "app/cmd.h"
#include "app/cmd_transaction.h"
#include "app/pref/preferences.h"
#include "doc/context.h"
#include "undo/undo_history.h"
#include "undo/undo_state.h"

#include <cassert>
#include <stdexcept>

namespace app {

DocumentUndo::DocumentUndo()
  : m_ctx(NULL)
  , m_savedCounter(0)
  , m_savedStateIsLost(false)
{
}

void DocumentUndo::setContext(doc::Context* ctx)
{
  m_ctx = ctx;
}

void DocumentUndo::add(CmdTransaction* cmd)
{
  ASSERT(cmd);

  // A linear undo history is the default behavior
  if (!App::instance() ||
      !App::instance()->preferences().undo.allowNonlinearHistory()) {
    m_undoHistory.clearRedo();
  }

  m_undoHistory.add(cmd);
}

bool DocumentUndo::canUndo() const
{
  return m_undoHistory.canUndo();
}

bool DocumentUndo::canRedo() const
{
  return m_undoHistory.canRedo();
}

void DocumentUndo::undo()
{
  return m_undoHistory.undo();
}

void DocumentUndo::redo()
{
  return m_undoHistory.redo();
}

void DocumentUndo::clearRedo()
{
  return m_undoHistory.clearRedo();
}

bool DocumentUndo::isSavedState() const
{
  return (!m_savedStateIsLost && m_savedCounter == 0);
}

void DocumentUndo::markSavedState()
{
  m_savedCounter = 0;
  m_savedStateIsLost = false;
}

void DocumentUndo::impossibleToBackToSavedState()
{
  m_savedStateIsLost = true;
}

std::string DocumentUndo::nextUndoLabel() const
{
  const undo::UndoState* state = nextUndo();
  if (state)
    return static_cast<Cmd*>(state->cmd())->label();
  else
    return "";
}

std::string DocumentUndo::nextRedoLabel() const
{
  const undo::UndoState* state = nextRedo();
  if (state)
    return static_cast<const Cmd*>(state->cmd())->label();
  else
    return "";
}

SpritePosition DocumentUndo::nextUndoSpritePosition() const
{
  const undo::UndoState* state = nextUndo();
  if (state)
    return static_cast<const CmdTransaction*>(state->cmd())
      ->spritePositionBeforeExecute();
  else
    return SpritePosition();
}

SpritePosition DocumentUndo::nextRedoSpritePosition() const
{
  const undo::UndoState* state = nextRedo();
  if (state)
    return static_cast<const CmdTransaction*>(state->cmd())
      ->spritePositionAfterExecute();
  else
    return SpritePosition();
}

Cmd* DocumentUndo::lastExecutedCmd() const
{
  const undo::UndoState* state = m_undoHistory.currentState();
  if (state)
    return static_cast<Cmd*>(state->cmd());
  else
    return NULL;
}

const undo::UndoState* DocumentUndo::nextUndo() const
{
  return m_undoHistory.currentState();
}

const undo::UndoState* DocumentUndo::nextRedo() const
{
  const undo::UndoState* state = m_undoHistory.currentState();
  if (state)
    return state->next();
  else
    return m_undoHistory.firstState();
}

} // namespace app
