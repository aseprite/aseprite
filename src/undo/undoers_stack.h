// ASEPRITE Undo Library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef UNDO_UNDOERS_STACK_H_INCLUDED
#define UNDO_UNDOERS_STACK_H_INCLUDED

#include "undo/modification.h"
#include "undo/undoers_collector.h"

#include <vector>

namespace undo {

class UndoHistory;
class Undoer;

// A stack of undoable actions (Undoers). There exist two stacks (see
// the UndoHistory class): One stack to hold actions to be undone (the
// "undoers stack"), and another stack were actions are held to redo
// reverted actions (the "redoers stack").
class UndoersStack : public UndoersCollector
{
public:
  enum PopFrom {
    PopFromHead,
    PopFromTail
  };

  // One item in the stack (label + Undoer)
  class Item
  {
  public:
    Item(const char* label, Modification mod, Undoer* undoer)
      : m_label(label)
      , m_mod(mod)
      , m_undoer(undoer) { }

    const char* getLabel() const { return m_label; }
    Modification getModification() const { return m_mod; }
    Undoer* getUndoer() const { return m_undoer; }

  private:
    const char* m_label;
    Modification m_mod;
    Undoer* m_undoer;
  };

  typedef std::vector<Item*> Items;
  typedef Items::iterator iterator;
  typedef Items::const_iterator const_iterator;

  // Ctor and dtor
  UndoersStack(UndoHistory* undoHistory);
  ~UndoersStack();

  UndoHistory* getUndoHistory() const { return m_undoHistory; }

  // Returns the collection of well-known serialized objects.
  ObjectsContainer* getObjects() const;

  iterator begin() { return m_items.begin(); }
  iterator end() { return m_items.end(); }
  const_iterator begin() const { return m_items.begin(); }
  const_iterator end() const { return m_items.end(); }
  bool empty() const { return m_items.empty(); }

  void clear();

  size_t getMemSize() const;

  // UndoersCollector implementation
  void pushUndoer(Undoer* undoer);

  // Removes a undoer from the stack, the returned undoer must be
  // deleted by the caller using Undoer::dispose().
  Undoer* popUndoer(PopFrom popFrom);

  size_t countUndoGroups() const;

private:
  UndoHistory* m_undoHistory;
  Items m_items;

  // Bytes occupied by all undoers in the stack.
  size_t m_size;
};

} // namespace undo

#endif  // UNDO_UNDOERS_STACK_H_INCLUDED
