// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/doc_undo.h"

#include "app/app.h"
#include "app/cmd.h"
#include "app/cmd_transaction.h"
#include "app/context.h"
#include "app/doc_undo_observer.h"
#include "app/pref/preferences.h"
#include "base/mem_utils.h"
#include "undo/undo_history.h"
#include "undo/undo_state.h"

#include <cassert>
#include <stdexcept>

#define UNDO_TRACE(...)
#define STATE_CMD(state) (static_cast<CmdTransaction*>(state->cmd()))

namespace app {

DocUndo::DocUndo()
  : m_undoHistory(this)
  , m_ctx(nullptr)
  , m_totalUndoSize(0)
  , m_savedCounter(0)
  , m_savedStateIsLost(false)
{
}

void DocUndo::setContext(Context* ctx)
{
  m_ctx = ctx;
}

void DocUndo::add(CmdTransaction* cmd)
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

  notify_observers(&DocUndoObserver::onAddUndoState, this);
  notify_observers(&DocUndoObserver::onTotalUndoSizeChange, this);

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

bool DocUndo::canUndo() const
{
  return m_undoHistory.canUndo();
}

bool DocUndo::canRedo() const
{
  return m_undoHistory.canRedo();
}

void DocUndo::undo()
{
  const undo::UndoState* state = nextUndo();
  ASSERT(state);
  const Cmd* cmd = STATE_CMD(state);
  size_t oldSize = m_totalUndoSize;
  m_totalUndoSize -= cmd->memSize();
  {
    m_undoHistory.undo();
    notify_observers(&DocUndoObserver::onCurrentUndoStateChange, this);
  }
  m_totalUndoSize += cmd->memSize();
  if (m_totalUndoSize != oldSize)
    notify_observers(&DocUndoObserver::onTotalUndoSizeChange, this);
}

void DocUndo::redo()
{
  const undo::UndoState* state = nextRedo();
  ASSERT(state);
  const Cmd* cmd = STATE_CMD(state);
  size_t oldSize = m_totalUndoSize;
  m_totalUndoSize -= cmd->memSize();
  {
    m_undoHistory.redo();
    notify_observers(&DocUndoObserver::onCurrentUndoStateChange, this);
  }
  m_totalUndoSize += cmd->memSize();
  if (m_totalUndoSize != oldSize)
    notify_observers(&DocUndoObserver::onTotalUndoSizeChange, this);
}

void DocUndo::clearRedo()
{
  m_undoHistory.clearRedo();
  notify_observers(&DocUndoObserver::onClearRedo, this);
}

bool DocUndo::isSavedState() const
{
  return (!m_savedStateIsLost && m_savedCounter == 0);
}

void DocUndo::markSavedState()
{
  m_savedCounter = 0;
  m_savedStateIsLost = false;
}

void DocUndo::impossibleToBackToSavedState()
{
  m_savedStateIsLost = true;
}

std::string DocUndo::nextUndoLabel() const
{
  const undo::UndoState* state = nextUndo();
  if (state)
    return STATE_CMD(state)->label();
  else
    return "";
}

std::string DocUndo::nextRedoLabel() const
{
  const undo::UndoState* state = nextRedo();
  if (state)
    return STATE_CMD(state)->label();
  else
    return "";
}

SpritePosition DocUndo::nextUndoSpritePosition() const
{
  const undo::UndoState* state = nextUndo();
  if (state)
    return STATE_CMD(state)->spritePositionBeforeExecute();
  else
    return SpritePosition();
}

SpritePosition DocUndo::nextRedoSpritePosition() const
{
  const undo::UndoState* state = nextRedo();
  if (state)
    return STATE_CMD(state)->spritePositionAfterExecute();
  else
    return SpritePosition();
}

std::istream* DocUndo::nextUndoDocRange() const
{
  const undo::UndoState* state = nextUndo();
  if (state)
    return STATE_CMD(state)->documentRangeBeforeExecute();
  else
    return nullptr;
}

std::istream* DocUndo::nextRedoDocRange() const
{
  const undo::UndoState* state = nextRedo();
  if (state)
    return STATE_CMD(state)->documentRangeAfterExecute();
  else
    return nullptr;
}

Cmd* DocUndo::lastExecutedCmd() const
{
  const undo::UndoState* state = m_undoHistory.currentState();
  if (state)
    return STATE_CMD(state);
  else
    return NULL;
}

void DocUndo::moveToState(const undo::UndoState* state)
{
  m_undoHistory.moveTo(state);
  notify_observers(&DocUndoObserver::onCurrentUndoStateChange, this);

  // Recalculate the total undo size
  size_t oldSize = m_totalUndoSize;
  m_totalUndoSize = 0;
  const undo::UndoState* s = m_undoHistory.firstState();
  while (s) {
    m_totalUndoSize += STATE_CMD(s)->memSize();
    s = s->next();
  }
  if (m_totalUndoSize != oldSize)
    notify_observers(&DocUndoObserver::onTotalUndoSizeChange, this);
}

const undo::UndoState* DocUndo::nextUndo() const
{
  return m_undoHistory.currentState();
}

const undo::UndoState* DocUndo::nextRedo() const
{
  const undo::UndoState* state = m_undoHistory.currentState();
  if (state)
    return state->next();
  else
    return m_undoHistory.firstState();
}

void DocUndo::onDeleteUndoState(undo::UndoState* state)
{
  ASSERT(state);
  Cmd* cmd = STATE_CMD(state);

  UNDO_TRACE("UNDO: Deleting undo state <%s> of %s from %s\n",
             cmd->label().c_str(),
             base::get_pretty_memory_size(cmd->memSize()).c_str(),
             base::get_pretty_memory_size(m_totalUndoSize).c_str());

  m_totalUndoSize -= cmd->memSize();
  notify_observers(&DocUndoObserver::onDeleteUndoState, this, state);
}

} // namespace app
