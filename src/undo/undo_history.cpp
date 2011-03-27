// ASEPRITE Undo Library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "undo/undo_history.h"

#include "undo/objects_container.h"
#include "undo/undoer.h"
#include "undo/undoers_stack.h"

#include <allegro/config.h>	// TODO remove this when get_config_int() is removed from here

using namespace undo;

UndoHistory::UndoHistory(ObjectsContainer* objects)
  : m_objects(objects)
{
  m_groupLevel = 0;
  m_diffCount = 0;
  m_diffSaved = 0;
  m_enabled = true;
  m_label = NULL;
  m_modification = ModifyDocument;

  m_undoers = new UndoersStack(this);
  try {
    m_redoers = new UndoersStack(this);
  }
  catch (...) {
    delete m_undoers;
    throw;
  }
}

UndoHistory::~UndoHistory()
{
  delete m_undoers;
  delete m_redoers;
}

bool UndoHistory::isEnabled() const
{
  return m_enabled ? true: false;
}

void UndoHistory::setEnabled(bool state)
{
  m_enabled = state;
}

bool UndoHistory::canUndo() const
{
  return !m_undoers->empty();
}

bool UndoHistory::canRedo() const
{
  return !m_redoers->empty();
}

void UndoHistory::doUndo()
{
  runUndo(UndoDirection);
}

void UndoHistory::doRedo()
{
  runUndo(RedoDirection);
}

void UndoHistory::clearRedo()
{
  if (!m_redoers->empty())
    m_redoers->clear();

  // Check if the saved state was in the redoers, in this case we've
  // just removed the saved state and now it is impossible to reach
  // that state again, so we have to put a value in m_diffSaved
  // impossible to be equal to m_diffCount.
  if (m_diffCount < m_diffSaved)
    m_diffSaved = -1;
}

const char* UndoHistory::getLabel()
{
  return m_label;
}

void UndoHistory::setLabel(const char* label)
{
  m_label = label;
}

Modification UndoHistory::getModification()
{
  return m_modification;
}

void UndoHistory::setModification(Modification mod)
{
  m_modification = mod;
}

const char* UndoHistory::getNextUndoLabel() const
{
  ASSERT(canUndo());

  UndoersStack::Item* item = *m_undoers->begin();
  return item->getLabel();
}

const char* UndoHistory::getNextRedoLabel() const
{
  ASSERT(canRedo());

  UndoersStack::Item* item = *m_redoers->begin();
  return item->getLabel();
}

bool UndoHistory::isSavedState() const
{
  return (m_diffCount == m_diffSaved);
}

void UndoHistory::markSavedState()
{
  m_diffSaved = m_diffCount;
}

void UndoHistory::runUndo(Direction direction)
{
  UndoersStack* undoers = ((direction == UndoDirection)? m_undoers: m_redoers);
  UndoersStack* redoers = ((direction == RedoDirection)? m_undoers: m_redoers);
  int level = 0;

  do {
    const char* itemLabel = NULL;
    Modification itemModification = DoesntModifyDocument;

    if (!undoers->empty()) {
      UndoersStack::Item* item = *undoers->begin();
      itemLabel = item->getLabel();
      itemModification = item->getModification();
    }

    Undoer* undoer = undoers->popUndoer(UndoersStack::PopFromHead);
    if (!undoer)
      break;

    setLabel(itemLabel);
    setModification(itemModification);

    undoer->revert(getObjects(), redoers);

    if (undoer->isOpenGroup())
      level++;
    else if (undoer->isCloseGroup())
      level--;

    // Delete the undoer
    undoer->dispose();

    // Adjust m_diffCount (just one time, when the level backs to zero)
    if (level == 0 && itemModification == ModifyDocument) {
      if (direction == UndoDirection)
	m_diffCount--;
      else if (direction == RedoDirection)
	m_diffCount++;
    }
  } while (level);
}

void UndoHistory::discardTail()
{
  int level = 0;

  do {
    Undoer* undoer = m_undoers->popUndoer(UndoersStack::PopFromTail);
    if (!undoer)
      break;

    if (undoer->isOpenGroup())
      level++;
    else if (undoer->isCloseGroup())
      level--;

    undoer->dispose();
  } while (level);
}

void UndoHistory::pushUndoer(Undoer* undoer)
{
  // TODO Replace this with the following implementation:
  // * Add the undo limit to UndoHistory class as a normal member (non-static).
  // * Add App signals to listen changes in settings
  // * Document should listen changes in the undo limit, 
  // * When a change is produced, Document calls getUndoHistory()->setUndoLimit().
  int undo_size_limit = (int)get_config_int("Options", "UndoSizeLimit", 8)*1024*1024;

  // Add the undoer in the undoers stack
  m_undoers->pushUndoer(undoer);

  // Reset the "redo" stack.
  clearRedo();

  // Adjust m_groupLevel
  if (undoer->isOpenGroup()) {
    ++m_groupLevel;
  }
  else if (undoer->isCloseGroup()) {
    --m_groupLevel;
  }

  // If we are outside a group, we can shrink the tail of the undo if
  // it has passed the undo-limit.
  if (m_groupLevel == 0) {
    // More differences.
    if (m_modification == ModifyDocument)
      m_diffCount++;

    // Is undo history too big?
    int groups = m_undoers->countUndoGroups();
    while (groups > 1 && m_undoers->getMemSize() > undo_size_limit) {
      discardTail();
      groups--;
    }
  }
}
