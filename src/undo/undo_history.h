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

class UndoHistory : public UndoersCollector
{
public:
  UndoHistory(ObjectsContainer* objects, UndoConfigProvider* configProvider);
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

  ObjectsContainer* getObjects() const { return m_objects; }

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
  size_t getUndoSizeLimit();

  ObjectsContainer* m_objects;  // Container of objects to insert & retrieve objects by ID
  UndoersStack* m_undoers;
  UndoersStack* m_redoers;
  int m_groupLevel;
  int m_diffCount;
  int m_diffSaved;
  UndoConfigProvider* m_configProvider;
};

} // namespace undo

#endif  // UNDO_UNDO_HISTORY_H_INCLUDED
