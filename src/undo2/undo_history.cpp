// Aseprite Undo2 Library
// Copyright (C) 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "undo2/undo_history.h"

#include "undo2/undo_command.h"
#include "undo2/undo_state.h"

#include <cassert>
#include <stack>

namespace undo2 {

UndoHistory::UndoHistory()
  : m_first(nullptr)
  , m_last(nullptr)
  , m_cur(nullptr)
  , m_createBranches(true)
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

void UndoHistory::setCreateBranches(bool state)
{
  m_createBranches = false;
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
  if (!m_createBranches)
    clearRedo();

  UndoState* state = new UndoState;
  state->m_prev = m_last;
  state->m_next = nullptr;
  state->m_parent = m_cur;
  state->m_cmd = cmd;

  if (!m_first)
    m_first = state;

  m_cur = m_last = state;

  if (state->m_prev) {
    assert(!state->m_prev->m_next);
    state->m_prev->m_next = state;
  }
}

UndoState* UndoHistory::findCommonParent(UndoState* a, UndoState* b)
{
  UndoState* pA = a;
  UndoState* pB = b;

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

void UndoHistory::moveTo(UndoState* new_state)
{
  UndoState* common = findCommonParent(m_cur, new_state);

  if (m_cur) {
    while (m_cur != common) {
      m_cur->m_cmd->undo();
      m_cur = m_cur->m_parent;
    }
  }

  if (new_state) {
    std::stack<UndoState*> redo_parents;
    UndoState* p = new_state;
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

  m_cur = new_state;
}

} // namespace undo
