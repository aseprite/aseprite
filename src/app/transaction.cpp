// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
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
#include "app/i18n/strings.h"
#include "app/modules/palettes.h"
#include "doc/sprite.h"
#include "ui/manager.h"
#include "ui/system.h"

#define TX_TRACE(...)

namespace app {

using namespace doc;

CannotModifyWhenReadOnlyException::CannotModifyWhenReadOnlyException() throw()
  : base::Exception(Strings::statusbar_tips_cannot_modify_readonly_sprite())
{
}

Transaction::Transaction(
  Context* ctx,
  Doc* doc,
  const std::string& label,
  Modification modification)
  : m_ctx(ctx)
  , m_doc(doc)
  , m_undo(nullptr)
  , m_cmds(nullptr)
  , m_changes(Changes::kNone)
{
  TX_TRACE("TX: Start <%s> (%s)\n",
           label.c_str(),
           modification == ModifyDocument ? "modifies document":
                                            "doesn't modify document");

  m_doc->add_observer(this);
  m_undo = m_doc->undoHistory();

  m_cmds = new CmdTransaction(label,
                              modification == Modification::ModifyDocument);

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
      rollback(nullptr);
  }
  catch (...) {
    // Just avoid throwing an exception in the dtor (just in case
    // rollback() failed).

    // TODO logging error
  }

  m_doc->remove_observer(this);
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
  // This assert can fail when we run scripts in batch mode
  //ui::assert_ui_thread();

  ASSERT(m_cmds);
  TX_TRACE("TX: Commit <%s>\n", m_cmds->label().c_str());

  m_cmds->updateSpritePositionAfter();
#ifdef ENABLE_UI
  const SpritePosition sprPos = m_cmds->spritePositionAfterExecute();
#endif

  m_undo->add(m_cmds);
  m_cmds = nullptr;

  // Process changes
  if (int(m_changes) & int(Changes::kSelection)) {
    m_doc->resetTransformation();
    m_doc->generateMaskBoundaries();
  }

#ifdef ENABLE_UI
  if (int(m_changes) & int(Changes::kColorChange)) {
    ASSERT(m_doc);
    ASSERT(m_doc->sprite());

    Palette* pal = m_doc->sprite()->palette(sprPos.frame());
    ASSERT(pal);
    if (pal)
      set_current_palette(pal, false);
    else
      set_current_palette(nullptr, false);

    if (m_ctx->isUIAvailable())
      ui::Manager::getDefault()->invalidate();
  }
#endif
}

void Transaction::rollbackAndStartAgain()
{
  auto newCmds = m_cmds->moveToEmptyCopy();
  rollback(newCmds);
  newCmds->execute(m_ctx);
}

void Transaction::rollback(CmdTransaction* newCmds)
{
  ASSERT(m_cmds);
  TX_TRACE("TX: Rollback <%s>\n", m_cmds->label().c_str());

  m_cmds->undo();

  delete m_cmds;
  m_cmds = newCmds;
}

void Transaction::execute(Cmd* cmd)
{
  // Read-only sprites cannot be modified.
  if (m_doc->isReadOnly()) {
    delete cmd;
    throw CannotModifyWhenReadOnlyException();
  }

  // If we are undoing/redoing, just throw an exception, we cannot
  // modify the sprite while we are moving throw the undo history.
  // To undo/redo we have just to call the onUndo/onRedo of each
  // app::Cmd.
  if (m_doc->isUndoing()) {
    delete cmd;
    throw CannotModifyWhenUndoingException();
  }

  try {
    // We have to add the "cmd" to the sequence (CmdTransaction) and
    // then execute it. This is because the execution can generate
    // some signals that could add/execute new actions to the undo
    // history/sequence.
    m_cmds->addAndExecute(m_ctx, cmd);
  }
  catch (...) {
    delete cmd;
    throw;
  }
}

void Transaction::onSelectionChanged(DocEvent& ev)
{
  m_changes = Changes(int(m_changes) | int(Changes::kSelection));
}

void Transaction::onColorSpaceChanged(DocEvent& ev)
{
  m_changes = Changes(int(m_changes) | int(Changes::kColorChange));
}

void Transaction::onPaletteChanged(DocEvent& ev)
{
  m_changes = Changes(int(m_changes) | int(Changes::kColorChange));
}

} // namespace app
