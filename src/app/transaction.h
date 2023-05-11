// Aseprite
// Copyright (C) 2019-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_TRANSACTION_H_INCLUDED
#define APP_TRANSACTION_H_INCLUDED
#pragma once

#include "app/cmd_transaction.h"
#include "app/doc_observer.h"
#include "base/exception.h"

#include <string>

namespace app {

  class Cmd;
  class Context;
  class DocRange;
  class DocUndo;

  enum Modification {
    ModifyDocument,      // This item changes the "saved status" of the document.
    DoesntModifyDocument // This item doesn't modify the document.
  };

  // Exception thrown when we want to modify a sprite (add new
  // app::Cmd objects) marked as read-only.
  class CannotModifyWhenReadOnlyException : public base::Exception {
  public:
    CannotModifyWhenReadOnlyException() throw();
  };

  // High-level class to group a set of commands to modify the
  // document atomically, with enough information to rollback the
  // whole operation if something fails (e.g. an exceptions is thrown)
  // in the middle of the procedure.
  //
  // This class is a DocObserver because it listen and accumulates the
  // changes in the Doc (m_changes), and when the transaction ends, it
  // processes those changes as UI updates (so widgets are
  // invalidated/updated correctly to show the new Doc state).
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
  class Transaction : public DocObserver {
  public:
    // Starts a undoable sequence of operations in a transaction that
    // can be committed or rollbacked.  All the operations will be
    // grouped in the sprite's undo as an atomic operation.
    Transaction(
      Context* ctx,
      Doc* doc,
      const std::string& label,
      Modification mod = ModifyDocument);
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

    // Discard everything that was added so far. We can start
    // executing new Cmds again.
    void rollbackAndStartAgain();

    // Executes the given command and tries to add it to the container
    // of executed commands (m_cmds).
    //
    // If some of these operations fails, the given "cmd" will be
    // deleted anyway.
    //
    // TODO In the future we should refactor this using unique
    //      pointers-like structure only
    void execute(Cmd* cmd);

    CmdTransaction* cmds() { return m_cmds; }

  private:
    // List of changes during the execution of this transaction
    enum class Changes {
      kNone = 0,
      // The selection has changed so we have to re-generate the
      // boundary segments.
      kSelection = 1,
      // The color palette or color space has changed.
      kColorChange = 2
    };

    void rollback(CmdTransaction* newCmds);

    // DocObserver impl
    void onSelectionChanged(DocEvent& ev) override;
    void onColorSpaceChanged(DocEvent& ev) override;
    void onPaletteChanged(DocEvent& ev) override;

    Context* m_ctx;
    Doc* m_doc;
    DocUndo* m_undo;
    CmdTransaction* m_cmds;
    Changes m_changes;
  };

} // namespace app

#endif
