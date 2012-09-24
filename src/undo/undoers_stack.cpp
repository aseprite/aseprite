// ASEPRITE Undo Library
// Copyright (C) 2001-2012  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "undo/undoers_stack.h"

#include "undo/undo_history.h"
#include "undo/undoer.h"

using namespace undo;

UndoersStack::UndoersStack(UndoHistory* undoHistory)
{
  m_undoHistory = undoHistory;
  m_size = 0;
}

UndoersStack::~UndoersStack()
{
  clear();
}

void UndoersStack::clear()
{
  for (iterator it = begin(), end = this->end(); it != end; ++it)
    (*it)->dispose();           // Delete the Undoer.

  m_size = 0;
  m_items.clear();              // Clear the list of items.
}

size_t UndoersStack::getMemSize() const
{
  return m_size;
}

ObjectsContainer* UndoersStack::getObjects() const
{
  return m_undoHistory->getObjects();
}

void UndoersStack::pushUndoer(Undoer* undoer)
{
  ASSERT(undoer != NULL);

  try {
    m_items.insert(begin(), undoer);
  }
  catch (...) {
    undoer->dispose();
    throw;
  }

  m_size += undoer->getMemSize();
}

Undoer* UndoersStack::popUndoer(PopFrom popFrom)
{
  Undoer* undoer;
  iterator it;

  if (!empty()) {
    if (popFrom == PopFromHead)
      it = begin();
    else
      it = --end();

    undoer = (*it);                 // Set the undoer to return.
    m_items.erase(it);              // Erase the item from the stack.
    m_size -= undoer->getMemSize(); // Reduce the stack size.
  }
  else
    undoer = NULL;

  return undoer;
}

size_t UndoersStack::countUndoGroups() const
{
  size_t groups = 0;
  int level;

  const_iterator it = begin();
  while (it != end()) {
    level = 0;

    do {
      const Undoer* undoer = (*it);
      ++it;

      if (undoer->isOpenGroup())
        level++;
      else if (undoer->isCloseGroup())
        level--;
    } while (level && (it != end()));

    if (level == 0)
      groups++;
  }

  return groups;
}
