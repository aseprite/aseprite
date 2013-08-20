// Aseprite Undo Library
// Copyright (C) 2001-2013 David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef UNDO_UNDOER_H_INCLUDED
#define UNDO_UNDOER_H_INCLUDED

#include "undo/modification.h"

namespace undo {

  class ObjectsContainer;
  class UndoersCollector;

  // Generic interface to undo/revert an action.
  class Undoer {
  public:
    virtual ~Undoer() { }

    // Used to destroy the undoer when it is not needed anymore. A
    // undoer is added in UndoersCollector, and then destroyed by
    // UndoHistory using this method.
    //
    // This method is available because the Undo library does not know
    // how this Undoer was created. So we cannot call just "delete undoer".
    virtual void dispose() = 0;

    // Returns the amount of memory (in bytes) which this instance is
    // using to revert the action.
    virtual size_t getMemSize() const = 0;

    // Returns the kind of modification that this item does with the
    // document.
    virtual Modification getModification() const = 0;

    // Returns true if this undoer is the first action of a group.
    virtual bool isOpenGroup() const = 0;

    // Returns true if this undoer is the last action of a group.
    virtual bool isCloseGroup() const = 0;

    // Reverts the action and adds to the "redoers" stack other set of
    // actions to redo the reverted action. It is the main method used
    // to undo any action.
    virtual void revert(ObjectsContainer* objects, UndoersCollector* redoers) = 0;
  };

} // namespace undo

#endif  // UNDO_UNDOER_H_INCLUDED
