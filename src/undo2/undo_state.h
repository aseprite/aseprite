// Aseprite Undo2 Library
// Copyright (C) 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UNDO2_UNDO_STATE_H_INCLUDED
#define UNDO2_UNDO_STATE_H_INCLUDED
#pragma once

namespace undo2 {

  class UndoCommand;
  class UndoHistory;

  // Represents a state that can be undone. If we are in this state,
  // is because the command was already executed.
  class UndoState {
    friend class UndoHistory;
  public:
    UndoState* prev() { return m_prev; }
    UndoState* next() { return m_next; }
    UndoCommand* cmd() { return m_cmd; }
  private:
    UndoState* m_prev;
    UndoState* m_next;
    UndoState* m_parent;             // Parent state, after we undo
    UndoCommand* m_cmd;
  };

} // namespace undo2

#endif  // UNDO2_UNDO_STATE_H_INCLUDED
