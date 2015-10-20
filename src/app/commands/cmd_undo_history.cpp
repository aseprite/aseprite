// Aseprite
// Copyright (C) 2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd.h"
#include "app/commands/command.h"
#include "app/console.h"
#include "app/context.h"
#include "app/document.h"
#include "app/document_access.h"
#include "app/document_undo.h"
#include "app/document_undo_observer.h"
#include "base/bind.h"
#include "doc/context_observer.h"
#include "doc/documents_observer.h"
#include "doc/site.h"
#include "undo/undo_state.h"

#include "undo_history.xml.h"

namespace app {

class UndoHistoryWindow : public app::gen::UndoHistory,
                          public doc::ContextObserver,
                          public doc::DocumentsObserver,
                          public app::DocumentUndoObserver {
public:
  class Item : public ui::ListItem {
  public:
    Item(const undo::UndoState* state)
      : ui::ListItem(
          (state ? static_cast<Cmd*>(state->cmd())->label():
                   std::string("Initial State"))),
        m_state(state) {
    }
    const undo::UndoState* state() { return m_state; }
  private:
    const undo::UndoState* m_state;
  };

  UndoHistoryWindow(Context* ctx)
    : m_ctx(ctx),
      m_document(nullptr) {
    actions()->Change.connect(&UndoHistoryWindow::onChangeAction, this);
  }

  ~UndoHistoryWindow() {
  }

private:
  bool onProcessMessage(ui::Message* msg) override {
    switch (msg->type()) {

      case ui::kOpenMessage:
        m_ctx->addObserver(this);
        m_ctx->documents().addObserver(this);
        if (m_ctx->activeDocument()) {
          attachDocument(
            static_cast<app::Document*>(m_ctx->activeDocument()));
        }
        break;

      case ui::kCloseMessage:
        if (m_document)
          detachDocument();
        m_ctx->documents().removeObserver(this);
        m_ctx->removeObserver(this);
        break;
    }
    return app::gen::UndoHistory::onProcessMessage(msg);
  }

  void onChangeAction() {
    Item* item = static_cast<Item*>(
      actions()->getSelectedChild());

    if (m_document &&
        m_document->undoHistory()->currentState() != item->state()) {
      try {
        DocumentWriter writer(m_document, 100);
        m_document->undoHistory()->moveToState(item->state());
        m_document->generateMaskBoundaries();
        m_document->notifyGeneralUpdate();
      }
      catch (const std::exception& ex) {
        selectState(m_document->undoHistory()->currentState());
        Console::showException(ex);
      }
    }
  }

  // ContextObserver
  void onActiveSiteChange(const doc::Site& site) override {
    if (m_document == site.document())
      return;

    attachDocument(
      static_cast<app::Document*>(
        const_cast<doc::Document*>(site.document())));
  }

  // DocumentsObserver
  void onRemoveDocument(doc::Document* doc) override {
    if (m_document && m_document == doc)
      detachDocument();
  }

  // DocumentUndoObserver
  void onAddUndoState(DocumentUndo* history) override {
    ASSERT(history->currentState());
    Item* item = new Item(history->currentState());
    actions()->addChild(item);
    actions()->layout();
    view()->updateView();
    actions()->selectChild(item);
  }

  void onAfterUndo(DocumentUndo* history) override {
    selectState(history->currentState());
  }

  void onAfterRedo(DocumentUndo* history) override {
    selectState(history->currentState());
  }

  void onClearRedo(DocumentUndo* history) override {
    refillList(history);
  }

  void attachDocument(app::Document* document) {
    detachDocument();

    m_document = document;
    if (!document)
      return;

    DocumentUndo* history = m_document->undoHistory();
    history->addObserver(this);

    refillList(history);
  }

  void detachDocument() {
    if (!m_document)
      return;

    clearList();
    m_document->undoHistory()->removeObserver(this);
    m_document = nullptr;
  }

  void clearList() {
    ui::Widget* child;
    while ((child = actions()->getFirstChild()))
      delete child;

    actions()->layout();
    view()->updateView();
  }

  void refillList(DocumentUndo* history) {
    clearList();

    // Create an item to reference the initial state (undo state == nullptr)
    Item* current = new Item(nullptr);
    actions()->addChild(current);

    const undo::UndoState* state = history->firstState();
    while (state) {
      Item* item = new Item(state);
      actions()->addChild(item);
      if (state == history->currentState())
        current = item;

      state = state->next();
    }

    actions()->layout();
    view()->updateView();
    if (current)
      actions()->selectChild(current);
  }

  void selectState(const undo::UndoState* state) {
    for (auto child : actions()->getChildren()) {
      Item* item = static_cast<Item*>(child);
      if (item->state() == state) {
        actions()->selectChild(item);
        break;
      }
    }
  }

  Context* m_ctx;
  app::Document* m_document;
};

class UndoHistoryCommand : public Command {
public:
  UndoHistoryCommand();
  Command* clone() const override { return new UndoHistoryCommand(*this); }

protected:
  void onExecute(Context* ctx) override;
};

static UndoHistoryWindow* g_window = NULL;

UndoHistoryCommand::UndoHistoryCommand()
  : Command("UndoHistory",
            "Undo History",
            CmdUIOnlyFlag)
{
}

void UndoHistoryCommand::onExecute(Context* ctx)
{
  if (!g_window)
    g_window = new UndoHistoryWindow(ctx);

  if (g_window->isVisible())
    g_window->setVisible(false);
  else
    g_window->openWindow();
}

Command* CommandFactory::createUndoHistoryCommand()
{
  return new UndoHistoryCommand;
}

} // namespace app
