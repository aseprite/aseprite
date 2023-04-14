// Aseprite
// Copyright (C) 2022-2023  Igara Studio S.A.
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
#include "app/console.h"
#include "app/context.h"
#include "app/doc_undo_observer.h"
#include "app/pref/preferences.h"
#include "base/mem_utils.h"
#include "base/scoped_value.h"
#include "undo/undo_history.h"
#include "undo/undo_state.h"

#include <cassert>
#include <stdexcept>

#define UNDO_TRACE(...)
#define STATE_CMD(state) (static_cast<CmdTransaction*>(state->cmd()))

namespace app {

DocUndo::DocUndo()
  : m_undoHistory(this)
{
}

void DocUndo::setContext(Context* ctx)
{
  m_ctx = ctx;
}

void DocUndo::add(CmdTransaction* cmd)
{
  ASSERT(cmd);

  if (m_undoing) {
    delete cmd;
    throw CannotModifyWhenUndoingException();
  }

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
  ASSERT(!m_undoing);
  base::ScopedValue undoing(m_undoing, true);
  const size_t oldSize = m_totalUndoSize;
  {
    const undo::UndoState* state = nextUndo();
    ASSERT(state);
    const Cmd* cmd = STATE_CMD(state);
    m_totalUndoSize -= cmd->memSize();
    m_undoHistory.undo();
    m_totalUndoSize += cmd->memSize();
  }
  // This notification could execute a script that modifies the sprite
  // again (e.g. a script that is listening the "change" event, check
  // the SpriteEvents class). If the sprite is modified, the "cmd" is
  // not valid anymore.
  notify_observers(&DocUndoObserver::onCurrentUndoStateChange, this);
  if (m_totalUndoSize != oldSize)
    notify_observers(&DocUndoObserver::onTotalUndoSizeChange, this);
}

void DocUndo::redo()
{
  ASSERT(!m_undoing);
  base::ScopedValue undoing(m_undoing, true);
  const size_t oldSize = m_totalUndoSize;
  {
    const undo::UndoState* state = nextRedo();
    ASSERT(state);
    const Cmd* cmd = STATE_CMD(state);
    m_totalUndoSize -= cmd->memSize();
    m_undoHistory.redo();
    m_totalUndoSize += cmd->memSize();
  }
  notify_observers(&DocUndoObserver::onCurrentUndoStateChange, this);
  if (m_totalUndoSize != oldSize)
    notify_observers(&DocUndoObserver::onTotalUndoSizeChange, this);
}

void DocUndo::clearRedo()
{
  // Do nothing
  if (currentState() == lastState())
    return;

  m_undoHistory.clearRedo();
  notify_observers(&DocUndoObserver::onClearRedo, this);
}

bool DocUndo::isInSavedStateOrSimilar() const
{
  if (m_savedStateIsLost)
    return false;

  // Here we try to find if we can reach the saved state from the
  // currentState() undoing or redoing and the sprite is exactly the
  // same as the saved state, e.g. this can happen if the undo states
  // don't modify the sprite (like actions that change the current
  // selection/mask boundaries).
  bool savedStateWithUndoes = true;

  auto state = currentState();
  while (state) {
    if (m_savedState == state) {
      return true;
    }
    else if (STATE_CMD(state)->doesChangeSavedState()) {
      savedStateWithUndoes = false;
      break;
    }
    state = state->prev();
  }

  // If we reached the end of the undo history (e.g. because all undo
  // states do not modify the sprite), the only way to be in the saved
  // state is if the initial point of history is the saved state too
  // i.e. when m_savedState is nullptr (and m_savedStateIsLost is
  // false).
  if (savedStateWithUndoes && m_savedState == nullptr)
    return true;

  // Now we try with redoes.
  state = (currentState() ? currentState()->next(): firstState());
  while (state) {
    if (STATE_CMD(state)->doesChangeSavedState()) {
      return false;
    }
    else if (m_savedState == state) {
      return true;
    }
    state = state->next();
  }
  return false;
}

void DocUndo::markSavedState()
{
  m_savedState = currentState();
  m_savedStateIsLost = false;
  notify_observers(&DocUndoObserver::onNewSavedState, this);
}

void DocUndo::impossibleToBackToSavedState()
{
  // Now there is no state related to the disk state.
  m_savedState = nullptr;
  m_savedStateIsLost = true;
  notify_observers(&DocUndoObserver::onNewSavedState, this);
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
  ASSERT(!m_undoing);
  base::ScopedValue undoing(m_undoing, true);

  m_undoHistory.moveTo(state);

  // After onCurrentUndoStateChange don't use the "state" argument, it
  // might be deleted because some script might have modified the
  // sprite on its "change" event.
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

  // Mark this document as impossible to match the version on disk
  // because we're just going to delete the saved state.
  if (m_savedState == state)
    impossibleToBackToSavedState();
}

} // namespace app
