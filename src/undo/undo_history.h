// ASEPRITE Undo Library
// Copyright (C) 2001-2012  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef UNDO_UNDO_HISTORY_H_INCLUDED
#define UNDO_UNDO_HISTORY_H_INCLUDED

#include "undo/modification.h"
#include "undo/undoers_collector.h"

#include <vector>

namespace undo {

class ObjectsContainer;
class UndoersStack;
class UndoConfigProvider;

class UndoHistoryDelegate
{
public:
  virtual ~UndoHistoryDelegate() { }

  // Container of objects to insert & retrieve objects by ID
  virtual ObjectsContainer* getObjects() const = 0;

  // Returns the limit of undo history in bytes.
  virtual size_t getUndoSizeLimit() const = 0;
};

class UndoHistory : public UndoersCollector
{
public:
  UndoHistory(UndoHistoryDelegate* delegate);
  virtual ~UndoHistory();

  bool canUndo() const;
  bool canRedo() const;

  void doUndo();
  void doRedo();

  void clearRedo();

  Undoer* getNextUndoer();
  Undoer* getNextRedoer();

  bool isSavedState() const;
  void markSavedState();

  ObjectsContainer* getObjects() const { return m_delegate->getObjects(); }

  // UndoersCollector interface
  void pushUndoer(Undoer* undoer);

  // Special method to add new undoers inside the last added group.
  // Returns true if the undoer was added in a group.
  bool implantUndoerInLastGroup(Undoer* undoer);

private:
  enum Direction { UndoDirection, RedoDirection };

  void runUndo(Direction direction);
  void discardTail();
  void updateUndo();
  void postUndoerAddedEvent(Undoer* undoer);
  void checkSizeLimit();

  UndoHistoryDelegate* m_delegate;
  UndoersStack* m_undoers;
  UndoersStack* m_redoers;
  int m_groupLevel;
  int m_diffCount;
  int m_diffSaved;
};

} // namespace undo

#endif  // UNDO_UNDO_HISTORY_H_INCLUDED
