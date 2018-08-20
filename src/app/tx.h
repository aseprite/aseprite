// Aseprite
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_TX_H_INCLUDED
#define APP_TX_H_INCLUDED
#pragma once

#include "app/app.h"
#include "app/context.h"
#include "app/transaction.h"

namespace app {

  // Wrapper to create a new transaction or get the current
  // transaction in the context.
  class Tx {
  public:
    Tx(Context* ctx, const std::string& label = "Transaction", Modification mod = ModifyDocument)
      : m_ctx(ctx) {
      m_transaction = m_ctx->transaction();
      if (m_transaction)
        m_owner = false;
      else {
        m_transaction = new Transaction(m_ctx, label, mod);
        m_ctx->setTransaction(m_transaction);
        m_owner = true;
      }
    }

    // Use the default App context
    Tx(const std::string& label = "Transaction", Modification mod = ModifyDocument)
      : Tx(App::instance()->context(), label, mod) {
    }

    ~Tx() {
      if (m_owner) {
        m_ctx->setTransaction(nullptr);
        delete m_transaction;
      }
    }

    void commit() {
      if (m_owner)
        m_transaction->commit();
    }

    void operator()(Cmd* cmd) {
      m_transaction->execute(cmd);
    }

    operator Transaction&() {
      return *m_transaction;
    }

  private:
    Context* m_ctx;
    Transaction* m_transaction;
    bool m_owner;               // Owner of the transaction
  };

} // namespace app

#endif
