// Aseprite Undo Library
// Copyright (C) 2001-2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "undo/undo_history.h"

#include "undo/objects_container.h"
#include "undo/undoer.h"
#include "undo/undoers_stack.h"

#include <limits>

namespace undo {

UndoHistory::UndoHistory(UndoHistoryDelegate* delegate)
  : m_delegate(delegate)
{
  m_groupLevel = 0;
  m_diffCount = 0;
  m_diffSaved = 0;

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
    impossibleToBackToSavedState();
}

Undoer* UndoHistory::getNextUndoer()
{
  if (!m_undoers->empty())
    return *m_undoers->begin();
  else
    return NULL;
}

Undoer* UndoHistory::getNextRedoer()
{
  if (!m_redoers->empty())
    return *m_redoers->begin();
  else
    return NULL;
}

bool UndoHistory::isSavedState() const
{
  return (m_diffCount == m_diffSaved);
}

void UndoHistory::markSavedState()
{
  m_diffSaved = m_diffCount;
}

void UndoHistory::impossibleToBackToSavedState()
{
  m_diffSaved = -1;
}

void UndoHistory::runUndo(Direction direction)
{
  UndoersStack* undoers = ((direction == UndoDirection)? m_undoers: m_redoers);
  UndoersStack* redoers = ((direction == RedoDirection)? m_undoers: m_redoers);
  int level = 0;

  do {
    const char* itemLabel = NULL;

    Undoer* undoer = undoers->popUndoer(UndoersStack::PopFromHead);
    if (!undoer)
      break;

    Modification itemModification = DoesntModifyDocument;
    itemModification = undoer->getModification();

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
  // Add the undoer in the undoers stack
  m_undoers->pushUndoer(undoer);

  postUndoerAddedEvent(undoer);
}

bool UndoHistory::implantUndoerInLastGroup(Undoer* undoer)
{
  Undoer* lastUndoer = m_undoers->popUndoer(UndoersStack::PopFromHead);
  bool result;

  if (lastUndoer == NULL || !lastUndoer->isCloseGroup()) {
    if (lastUndoer)
      m_undoers->pushUndoer(lastUndoer);
    m_undoers->pushUndoer(undoer);

    result = false;
  }
  else {
    m_undoers->pushUndoer(undoer);
    m_undoers->pushUndoer(lastUndoer);
    result = true;
  }

  postUndoerAddedEvent(undoer);
  return result;
}

void UndoHistory::postUndoerAddedEvent(Undoer* undoer)
{
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
    if (undoer->getModification() == ModifyDocument)
      m_diffCount++;

    checkSizeLimit();
  }
}

// Discards undoers in in case the UndoHistory is bigger than the given limit.
void UndoHistory::checkSizeLimit()
{
  // Is undo history too big?
  size_t groups = m_undoers->countUndoGroups();
  size_t undoLimit = m_delegate->getUndoSizeLimit();
  while (groups > 1 && m_undoers->getMemSize() > undoLimit) {
    discardTail();
    groups--;
  }
}

} // namespace undo
