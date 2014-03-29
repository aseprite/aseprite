// Aseprite Undo Library
// Copyright (C) 2001-2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UNDO_UNDOERS_COLLECTOR_H_INCLUDED
#define UNDO_UNDOERS_COLLECTOR_H_INCLUDED
#pragma once

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

#endif  // UNDO_UNDOERS_COLLECTOR_H_INCLUDED
