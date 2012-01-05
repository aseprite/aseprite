// ASEPRITE Undo Library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef UNDO_UNDO_HISTORY_H_INCLUDED
#define UNDO_UNDO_HISTORY_H_INCLUDED

#include "undo/modification.h"
#include "undo/undoers_collector.h"

#include <vector>

namespace undo {

class ObjectsContainer;
class UndoersStack;

class UndoHistory : public UndoersCollector
{
public:
  UndoHistory(ObjectsContainer* objects);
  virtual ~UndoHistory();

  bool isEnabled() const;
  void setEnabled(bool state);

  bool canUndo() const;
  bool canRedo() const;

  void doUndo();
  void doRedo();

  void clearRedo();

  // Current label for next added Undoers.
  const char* getLabel();
  void setLabel(const char* label);

  // Change the "modify saved status" flag to be assigned for next
  // added items. When it is activated means that each added Undoer
  // modifies the "saved status" of the document.
  Modification getModification();
  void setModification(Modification mod);

  const char* getNextUndoLabel() const;
  const char* getNextRedoLabel() const;

  bool isSavedState() const;
  void markSavedState();

  ObjectsContainer* getObjects() const { return m_objects; }

  // UndoersCollector interface
  void pushUndoer(Undoer* undoer);

  // Special method to add new undoers inside the last added group.
  // Returns true if the undoer was added in a group.
  bool graftUndoerInLastGroup(Undoer* undoer);

private:
  enum Direction { UndoDirection, RedoDirection };

  void runUndo(Direction direction);
  void discardTail();
  void updateUndo();
  void postUndoerAddedEvent(Undoer* undoer);
  void checkSizeLimit();
  static int getUndoSizeLimit();

  ObjectsContainer* m_objects;  // Container of objects to insert & retrieve objects by ID
  UndoersStack* m_undoers;
  UndoersStack* m_redoers;
  int m_groupLevel;
  int m_diffCount;
  int m_diffSaved;
  bool m_enabled;               // Is undo enabled?
  const char* m_label;          // Current label to be applied to all next undo operations.
  Modification m_modification;  // Current label to be applied to all next undo operations.
};

} // namespace undo

#endif  // UNDO_UNDO_HISTORY_H_INCLUDED
