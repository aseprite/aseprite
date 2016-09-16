// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/document_undo.h"

#include "app/app.h"
#include "app/cmd.h"
#include "app/cmd_transaction.h"
#include "app/document_undo_observer.h"
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
    clearRedo();
  }

  m_undoHistory.add(cmd);
  notify_observers(&DocumentUndoObserver::onAddUndoState, this);
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
  m_undoHistory.undo();
  notify_observers(&DocumentUndoObserver::onAfterUndo, this);
}

void DocumentUndo::redo()
{
  m_undoHistory.redo();
  notify_observers(&DocumentUndoObserver::onAfterRedo, this);
}

void DocumentUndo::clearRedo()
{
  m_undoHistory.clearRedo();
  notify_observers(&DocumentUndoObserver::onClearRedo, this);
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

void DocumentUndo::moveToState(const undo::UndoState* state)
{
  m_undoHistory.moveTo(state);
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
