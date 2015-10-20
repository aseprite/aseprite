// Aseprite Undo Library
// Copyright (C) 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "undo/undo_history.h"

#include "undo/undo_command.h"
#include "undo/undo_state.h"

#include <cassert>
#include <stack>

namespace undo {

UndoHistory::UndoHistory()
  : m_first(nullptr)
  , m_last(nullptr)
  , m_cur(nullptr)
{
}

UndoHistory::~UndoHistory()
{
  m_cur = nullptr;
  clearRedo();
}

bool UndoHistory::canUndo() const
{
  return m_cur != nullptr;
}

bool UndoHistory::canRedo() const
{
  return m_cur != m_last;
}

void UndoHistory::undo()
{
  assert(m_cur);
  if (!m_cur)
    return;

  assert(
    (m_cur != m_first && m_cur->m_prev) ||
    (m_cur == m_first && !m_cur->m_prev));

  moveTo(m_cur->m_prev);
}

void UndoHistory::redo()
{
  if (!m_cur)
    moveTo(m_first);
  else
    moveTo(m_cur->m_next);
}

void UndoHistory::clearRedo()
{
  for (UndoState* state = m_last, *prev;
       state && state != m_cur;
       state = prev) {
    prev = state->m_prev;
    delete state;
  }

  if (m_cur) {
    m_cur->m_next = nullptr;
    m_last = m_cur;
  }
  else {
    m_first = m_last = nullptr;
  }
}

void UndoHistory::add(UndoCommand* cmd)
{
  UndoState* state = new UndoState(cmd);
  state->m_prev = m_last;
  state->m_next = nullptr;
  state->m_parent = m_cur;

  if (!m_first)
    m_first = state;

  m_cur = m_last = state;

  if (state->m_prev) {
    assert(!state->m_prev->m_next);
    state->m_prev->m_next = state;
  }
}

const UndoState* UndoHistory::findCommonParent(const UndoState* a,
                                               const UndoState* b)
{
  const UndoState* pA = a;
  const UndoState* pB = b;

  if (pA == nullptr || pB == nullptr)
    return nullptr;

  while (pA != pB) {
    pA = pA->m_parent;
    if (!pA) {
      pA = a;
      pB = pB->m_parent;
      if (!pB)
        return nullptr;
    }
  }

  return pA;
}

void UndoHistory::moveTo(const UndoState* new_state)
{
  const UndoState* common = findCommonParent(m_cur, new_state);

  if (m_cur) {
    while (m_cur != common) {
      m_cur->m_cmd->undo();
      m_cur = m_cur->m_parent;
    }
  }

  if (new_state) {
    std::stack<const UndoState*> redo_parents;
    const UndoState* p = new_state;
    while (p != common) {
      redo_parents.push(p);
      p = p->m_parent;
    }

    while (!redo_parents.empty()) {
      p = redo_parents.top();
      redo_parents.pop();

      p->m_cmd->redo();
    }
  }

  m_cur = const_cast<UndoState*>(new_state);
}

} // namespace undo
