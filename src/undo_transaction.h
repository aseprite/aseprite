/* ASEPRITE
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef UNDO_TRANSACTION_H_INCLUDED
#define UNDO_TRANSACTION_H_INCLUDED

#include "undo/modification.h"

class Context;
class Document;
class DocumentUndo;
class Sprite;

namespace undo {
  class ObjectsContainer;
  class Undoer;
}

// High-level class to group a set of operations to modify the
// document atomically, adding information in the undo history to
// rollback the whole operation if something fails (with an
// exceptions) in the middle of the procedure.
//
// You have to wrap every call to an undo transaction with a
// ContextWriter. The preferred usage is as the following:
//
// {
//   ContextWriter writer(context);
//   UndoTransaction undoTransaction(context, "My big operation");
//   ...
//   undoTransaction.commit();
// }
//
class UndoTransaction
{
public:

  // Starts a undoable sequence of operations in a transaction that
  // can be committed or rollbacked.  All the operations will be
  // grouped in the sprite's undo as an atomic operation.
  UndoTransaction(Context* context, const char* label, undo::Modification mod = undo::ModifyDocument);
  virtual ~UndoTransaction();

  inline bool isEnabled() const { return m_enabledFlag; }

  // This must be called to commit all the changes, so the undo will
  // be finally added in the sprite.
  //
  // If you don't use this routine, all the changes will be discarded
  // (if the sprite's undo was enabled when the UndoTransaction was
  // created).
  void commit();

  void pushUndoer(undo::Undoer* undoer);
  undo::ObjectsContainer* getObjects() const;

private:
  void closeUndoGroup();
  void rollback();

  Context* m_context;
  Document* m_document;
  Sprite* m_sprite;
  DocumentUndo* m_undo;
  bool m_closed;
  bool m_committed;
  bool m_enabledFlag;
  const char* m_label;
  undo::Modification m_modification;
};

#endif
