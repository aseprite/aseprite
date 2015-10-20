// Aseprite Undo Library
// Copyright (C) 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UNDO_UNDO_HISTORY_H_INCLUDED
#define UNDO_UNDO_HISTORY_H_INCLUDED
#pragma once

namespace undo {

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
    void clearRedo();

    // This can be used to jump to a specific UndoState in the whole
    // history.
    void moveTo(const UndoState* new_state);

  private:
    const UndoState* findCommonParent(const UndoState* a,
                                      const UndoState* b);

    UndoState* m_first;
    UndoState* m_last;
    UndoState* m_cur;          // Current action that can be undone
  };

} // namespace undo

#endif  // UNDO_UNDO_HISTORY_H_INCLUDED
