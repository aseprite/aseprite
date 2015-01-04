// Aseprite Undo2 Library
// Copyright (C) 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UNDO2_UNDO_HISTORY_H_INCLUDED
#define UNDO2_UNDO_HISTORY_H_INCLUDED
#pragma once

namespace undo2 {

  class UndoCommand;
  class UndoState;

  class UndoHistory {
  public:
    UndoHistory();
    virtual ~UndoHistory();

    const UndoState* firstState()   const { return m_first; }
    const UndoState* lastState()    const { return m_last; }
    const UndoState* currentState() const { return m_cur; }

    void add(UndoCommand* cmd);
    bool canUndo() const;
    bool canRedo() const;
    void undo();
    void redo();

    // By default, UndoHistory creates branches if we undo a command
    // and call add(). With this method we can disable this behavior
    // and call clearRedo() automatically when a new command is added
    // in the history. (Like a regular undo history works.)
    void setCreateBranches(bool state);
    void clearRedo();

  private:
    UndoState* findCommonParent(UndoState* a, UndoState* b);
    void moveTo(UndoState* new_state);

    UndoState* m_first;
    UndoState* m_last;
    UndoState* m_cur;          // Current action that can be undone
    bool m_createBranches;
  };

} // namespace undo2

#endif  // UNDO2_UNDO_HISTORY_H_INCLUDED
