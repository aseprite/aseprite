// ASEPRITE Undo Library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
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
  for (iterator it = begin(), end = this->end(); it != end; ++it) {
    (*it)->getUndoer()->dispose(); // Delete the Undoer.
    delete *it;                    // Delete the UndoersStack::Item.
  }

  m_size = 0;
  m_items.clear();              // Clear the list of items.
}

int UndoersStack::getMemSize() const
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
    Item* item = new Item(m_undoHistory->getLabel(),
                          m_undoHistory->getModification(), undoer);
    try {
      m_items.insert(begin(), item);
    }
    catch (...) {
      delete item;
      throw;
    }
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

    undoer = (*it)->getUndoer();    // Set the undoer to return.
    delete *it;                     // Delete the UndoersStack::Item.
    m_items.erase(it);              // Erase the item from the stack.
    m_size -= undoer->getMemSize(); // Reduce the stack size.
  }
  else
    undoer = NULL;

  return undoer;
}

int UndoersStack::countUndoGroups() const
{
  int groups = 0;
  int level;

  const_iterator it = begin();
  while (it != end()) {
    level = 0;

    do {
      const Undoer* undoer = (*it)->getUndoer();
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
