// Aseprite
// Copyright (C) 2001-2017  David Capello
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
#include "base/mem_utils.h"
#include "doc/context.h"
#include "undo/undo_history.h"
#include "undo/undo_state.h"

#include <cassert>
#include <stdexcept>

#define UNDO_TRACE(...)
#define STATE_CMD(state) (static_cast<CmdTransaction*>(state->cmd()))

namespace app {

DocumentUndo::DocumentUndo()
  : m_undoHistory(this)
  , m_ctx(nullptr)
  , m_totalUndoSize(0)
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
  UNDO_TRACE("UNDO: Add state <%s> of %s to %s\n",
             cmd->label().c_str(),
             base::get_pretty_memory_size(cmd->memSize()).c_str(),
             base::get_pretty_memory_size(m_totalUndoSize).c_str());

  // A linear undo history is the default behavior
  if (!App::instance() ||
      !App::instance()->preferences().undo.allowNonlinearHistory()) {
    clearRedo();
  }

  m_undoHistory.add(cmd);
  m_totalUndoSize += cmd->memSize();

  notify_observers(&DocumentUndoObserver::onAddUndoState, this);
  notify_observers(&DocumentUndoObserver::onTotalUndoSizeChange, this);

  if (App::instance()) {
    const size_t undoLimitSize =
      int(App::instance()->preferences().undo.sizeLimit())
      * 1024 * 1024;

    // If undo limit is 0, it means "no limit", so we ignore the
    // complete logic to discard undo states.
    if (undoLimitSize > 0 &&
        m_totalUndoSize > undoLimitSize) {
      UNDO_TRACE("UNDO: Reducing undo history from %s to %s\n",
                 base::get_pretty_memory_size(m_totalUndoSize).c_str(),
                 base::get_pretty_memory_size(undoLimitSize).c_str());

      while (m_undoHistory.firstState() &&
             m_totalUndoSize > undoLimitSize) {
        if (!m_undoHistory.deleteFirstState())
          break;
      }
    }
  }

  UNDO_TRACE("UNDO: New undo size %s\n",
             base::get_pretty_memory_size(m_totalUndoSize).c_str());
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
  const undo::UndoState* state = nextUndo();
  ASSERT(state);
  const Cmd* cmd = STATE_CMD(state);
  size_t oldSize = m_totalUndoSize;
  m_totalUndoSize -= cmd->memSize();
  {
    m_undoHistory.undo();
    notify_observers(&DocumentUndoObserver::onAfterUndo, this);
  }
  m_totalUndoSize += cmd->memSize();
  if (m_totalUndoSize != oldSize)
    notify_observers(&DocumentUndoObserver::onTotalUndoSizeChange, this);
}

void DocumentUndo::redo()
{
  const undo::UndoState* state = nextRedo();
  ASSERT(state);
  const Cmd* cmd = STATE_CMD(state);
  size_t oldSize = m_totalUndoSize;
  m_totalUndoSize -= cmd->memSize();
  {
    m_undoHistory.redo();
    notify_observers(&DocumentUndoObserver::onAfterRedo, this);
  }
  m_totalUndoSize += cmd->memSize();
  if (m_totalUndoSize != oldSize)
    notify_observers(&DocumentUndoObserver::onTotalUndoSizeChange, this);
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
    return STATE_CMD(state)->label();
  else
    return "";
}

std::string DocumentUndo::nextRedoLabel() const
{
  const undo::UndoState* state = nextRedo();
  if (state)
    return STATE_CMD(state)->label();
  else
    return "";
}

SpritePosition DocumentUndo::nextUndoSpritePosition() const
{
  const undo::UndoState* state = nextUndo();
  if (state)
    return STATE_CMD(state)->spritePositionBeforeExecute();
  else
    return SpritePosition();
}

SpritePosition DocumentUndo::nextRedoSpritePosition() const
{
  const undo::UndoState* state = nextRedo();
  if (state)
    return STATE_CMD(state)->spritePositionAfterExecute();
  else
    return SpritePosition();
}

Cmd* DocumentUndo::lastExecutedCmd() const
{
  const undo::UndoState* state = m_undoHistory.currentState();
  if (state)
    return STATE_CMD(state);
  else
    return NULL;
}

void DocumentUndo::moveToState(const undo::UndoState* state)
{
  m_undoHistory.moveTo(state);

  // Recalculate the total undo size
  size_t oldSize = m_totalUndoSize;
  m_totalUndoSize = 0;
  const undo::UndoState* s = m_undoHistory.firstState();
  while (s) {
    m_totalUndoSize += STATE_CMD(s)->memSize();
    s = s->next();
  }
  if (m_totalUndoSize != oldSize)
    notify_observers(&DocumentUndoObserver::onTotalUndoSizeChange, this);
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

void DocumentUndo::onDeleteUndoState(undo::UndoState* state)
{
  Cmd* cmd = STATE_CMD(state);

  UNDO_TRACE("UNDO: Deleting undo state <%s> of %s from %s\n",
             cmd->label().c_str(),
             base::get_pretty_memory_size(cmd->memSize()).c_str(),
             base::get_pretty_memory_size(m_totalUndoSize).c_str());

  m_totalUndoSize -= cmd->memSize();
  notify_observers(&DocumentUndoObserver::onDeleteUndoState, this, state);
}

} // namespace app
