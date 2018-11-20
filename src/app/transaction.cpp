// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/transaction.h"

#include "app/cmd_transaction.h"
#include "app/context_access.h"
#include "app/doc.h"
#include "app/doc_undo.h"
#include "doc/sprite.h"

#define TX_TRACE(...)

namespace app {

using namespace doc;

Transaction::Transaction(Context* ctx, const std::string& label, Modification modification)
  : m_ctx(ctx)
  , m_cmds(NULL)
{
  TX_TRACE("TX: Start <%s> (%s)\n",
           label.c_str(),
           modification == ModifyDocument ? "modifies document":
                                            "doesn't modify document");

  Doc* doc = m_ctx->activeDocument();
  if (!doc)
    throw std::runtime_error("No active document to execute a transaction");
  m_undo = doc->undoHistory();

  m_cmds = new CmdTransaction(label,
    modification == Modification::ModifyDocument,
    m_undo->savedCounter());

  // Here we are executing an empty CmdTransaction, just to save the
  // SpritePosition. Sub-cmds are executed then one by one, in
  // Transaction::execute()
  m_cmds->execute(m_ctx);
}

Transaction::~Transaction()
{
  try {
    // If it isn't committed, we have to rollback all changes.
    if (m_cmds)
      rollback();
  }
  catch (...) {
    // Just avoid throwing an exception in the dtor (just in case
    // rollback() failed).

    // TODO logging error
  }
}

// Used to set the document range after all the transaction is
// executed and before the commit. This range is stored in
// CmdTransaction to recover it on Edit > Redo.
void Transaction::setNewDocRange(const DocRange& range)
{
  ASSERT(m_cmds);
  m_cmds->setNewDocRange(range);
}

void Transaction::commit()
{
  ASSERT(m_cmds);
  TX_TRACE("TX: Commit <%s>\n", m_cmds->label().c_str());

  m_cmds->commit();
  m_undo->add(m_cmds);
  m_cmds = NULL;
}

void Transaction::rollback()
{
  ASSERT(m_cmds);
  TX_TRACE("TX: Rollback <%s>\n", m_cmds->label().c_str());

  m_cmds->undo();

  delete m_cmds;
  m_cmds = NULL;
}

void Transaction::execute(Cmd* cmd)
{
  try {
    cmd->execute(m_ctx);
  }
  catch (...) {
    delete cmd;
    throw;
  }

  try {
    m_cmds->add(cmd);
  }
  catch (...) {
    cmd->undo();
    delete cmd;
    throw;
  }
}

} // namespace app
