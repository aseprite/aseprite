// ASEPRITE Undo Library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef UNDO_UNDOERS_COLLECTOR_H_INCLUDED
#define UNDO_UNDOERS_COLLECTOR_H_INCLUDED

namespace undo {

class Undoer;
class ObjectsContainer;

// Simple interface to collect undoers. It's implemented by
// UndoHistory and UndoersStack.
//
// This class is passed to Undoer::revert() so the Undoer can add
// operations (other Undoers) to redo the reverted operation.
class UndoersCollector
{
public:
  virtual ~UndoersCollector() { }

  // Adds a new undoer into the collection. The undoer will be owned
  // by the collector, so it will be deleted automatically (using
  // Undoer::dispose() method).
  virtual void pushUndoer(Undoer* undoer) = 0;
};

} // namespace undo

#endif	// UNDO_UNDOERS_COLLECTOR_H_INCLUDED
