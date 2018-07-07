// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_TRANSACTION_H_INCLUDED
#define APP_TRANSACTION_H_INCLUDED
#pragma once

#include <string>

namespace app {

  class Cmd;
  class CmdTransaction;
  class Context;
  class DocRange;
  class DocUndo;

  enum Modification {
    ModifyDocument,      // This item changes the "saved status" of the document.
    DoesntModifyDocument // This item doesn't modify the document.
  };

  // High-level class to group a set of commands to modify the
  // document atomically, with enough information to rollback the
  // whole operation if something fails (e.g. an exceptions is thrown)
  // in the middle of the procedure.
  //
  // You have to wrap every call to an transaction with a
  // ContextWriter. The preferred usage is as follows:
  //
  // {
  //   ContextWriter writer(context);
  //   Transaction transaction(context, "My big operation");
  //   ...
  //   transaction.commit();
  // }
  //
  class Transaction {
  public:
    // Starts a undoable sequence of operations in a transaction that
    // can be committed or rollbacked.  All the operations will be
    // grouped in the sprite's undo as an atomic operation.
    Transaction(Context* ctx, const std::string& label, Modification mod = ModifyDocument);
    virtual ~Transaction();

    // Can be used to change the new document range resulting from
    // executing this transaction. This range can be used then in
    // undo/redo operations to restore the Timeline selection/range.
    void setNewDocRange(const DocRange& range);

    // This must be called to commit all the changes, so the undo will
    // be finally added in the sprite.
    //
    // If you don't use this routine, all the changes will be discarded
    // (if the sprite's undo was enabled when the Transaction was
    // created).
    //
    // WARNING: This must be called from the main UI thread, because
    // it will generate a DocUndo::add() which triggers a
    // DocUndoObserver::onAddUndoState() notification, which
    // updates the Undo History window UI.
    void commit();

    void execute(Cmd* cmd);

  private:
    void rollback();

    Context* m_ctx;
    DocUndo* m_undo;
    CmdTransaction* m_cmds;
  };

} // namespace app

#endif
