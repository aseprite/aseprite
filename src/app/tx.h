// Aseprite
// Copyright (C) 2019-2023  Igara Studio S.A.
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_TX_H_INCLUDED
#define APP_TX_H_INCLUDED
#pragma once

#include "app/app.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/doc.h"
#include "app/doc_access.h"
#include "app/transaction.h"
#include "doc/sprite.h"

#include <stdexcept>

namespace app {

  // Wrapper to create a new transaction or get the current
  // transaction in the context.
  class Tx {
  public:
    enum LockAction {
      DocIsLocked, // The doc is locked to be written
      LockDoc,     // We have to lock the doc in Tx() ctor to write it
      DontLockDoc, // In case that we are going for a step-by-step
                   // transaction (e.g. ToolLoop or PixelsMovement)
    };

    static constexpr const char* kDefaultTransactionName = "Transaction";

    Tx() = delete;

    Tx(const LockAction lockAction,
       Context* ctx,
       Doc* doc,
       const std::string& label = kDefaultTransactionName,
       const Modification mod = ModifyDocument)
    {
      m_doc = doc;
      if (!m_doc)
        throw std::runtime_error("No document to execute a transaction");

      // Lock the document here (even if we are inside a transaction,
      // this will be a re-entrant lock if we can)
      if (lockAction == LockDoc) {
        m_lockResult = m_doc->writeLock(500);
        if (m_lockResult == Doc::LockResult::Fail)
          throw CannotWriteDocException();
      }

      m_transaction = m_doc->transaction();
      if (m_transaction) {
        m_owner = false;
      }
      else {
        m_transaction = new Transaction(ctx, m_doc, label, mod);
        m_doc->setTransaction(m_transaction);
        m_owner = true;
      }
    }

  public:
    Tx(Doc* doc,
       const std::string& label = kDefaultTransactionName,
       const Modification mod = ModifyDocument)
      : Tx(LockDoc, doc->context(), doc, label, mod)
    {
    }

    Tx(doc::Sprite* spr,
       const std::string& label = kDefaultTransactionName,
       const Modification mod = ModifyDocument)
      : Tx(static_cast<Doc*>(spr->document()), label, mod)
    {
    }

    // Use active document of the given context
    Tx(ContextWriter& writer,
       const std::string& label = kDefaultTransactionName,
       const Modification mod = ModifyDocument)
      : Tx(DocIsLocked,
           writer.context(),
           writer.document(), label, mod)
    {
    }

    ~Tx() {
      if (m_owner) {
        m_doc->setTransaction(nullptr);
        delete m_transaction;
      }

      if (m_lockResult != Doc::LockResult::Fail)
        m_doc->unlock(m_lockResult);
    }

    void commit() {
      if (m_owner)
        m_transaction->commit();
    }

    void setNewDocRange(const DocRange& range) {
      m_transaction->setNewDocRange(range);
    }

    void rollbackAndStartAgain() {
      m_transaction->rollbackAndStartAgain();
    }

    // If the command cannot be executed, it will be deleted anyway.
    void operator()(Cmd* cmd) {
      m_transaction->execute(cmd);
    }

    operator Transaction&() {
      return *m_transaction;
    }

    operator CmdTransaction*() {
      return m_transaction->cmds();
    }

  private:
    Doc* m_doc;
    Transaction* m_transaction;
    Doc::LockResult m_lockResult = Doc::LockResult::Fail;
    bool m_owner = false;  // Owner of the transaction
  };

} // namespace app

#endif
