// Aseprite
// Copyright (C) 2015-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd.h"
#include "app/commands/command.h"
#include "app/console.h"
#include "app/context.h"
#include "app/context_observer.h"
#include "app/doc.h"
#include "app/doc_undo.h"
#include "app/doc_undo_observer.h"
#include "app/docs_observer.h"
#include "app/doc_access.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/site.h"
#include "base/bind.h"
#include "base/mem_utils.h"
#include "ui/listitem.h"
#include "ui/message.h"
#include "undo/undo_state.h"

#include "undo_history.xml.h"

namespace app {

class UndoHistoryWindow : public app::gen::UndoHistory,
                          public ContextObserver,
                          public DocsObserver,
                          public DocUndoObserver {
public:
  class Item : public ui::ListItem {
  public:
    Item(const undo::UndoState* state)
      : ui::ListItem(
          (state ?
           static_cast<Cmd*>(state->cmd())->label()
#if _DEBUG
           + std::string(" ") + base::get_pretty_memory_size(static_cast<Cmd*>(state->cmd())->memSize())
#endif
           : std::string("Initial State"))),
        m_state(state) {
    }
    const undo::UndoState* state() { return m_state; }
  private:
    const undo::UndoState* m_state;
  };

  UndoHistoryWindow(Context* ctx)
    : m_ctx(ctx)
    , m_document(nullptr) {
    m_title = text();
    actions()->Change.connect(&UndoHistoryWindow::onChangeAction, this);
  }

  ~UndoHistoryWindow() {
  }

private:
  bool onProcessMessage(ui::Message* msg) override {
    switch (msg->type()) {

      case ui::kOpenMessage:
        load_window_pos(this, "UndoHistory");

        m_ctx->add_observer(this);
        m_ctx->documents().add_observer(this);
        if (m_ctx->activeDocument()) {
          m_frame = m_ctx->activeSite().frame();

          attachDocument(m_ctx->activeDocument());
        }
        break;

      case ui::kCloseMessage:
        save_window_pos(this, "UndoHistory");

        if (m_document)
          detachDocument();
        m_ctx->documents().remove_observer(this);
        m_ctx->remove_observer(this);
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
        DocWriter writer(m_document, 100);
        m_document->undoHistory()->moveToState(item->state());
        m_document->generateMaskBoundaries();

        // TODO this should be an observer of the current document palette
        set_current_palette(m_document->sprite()->palette(m_frame),
                            false);

        m_document->notifyGeneralUpdate();
      }
      catch (const std::exception& ex) {
        selectState(m_document->undoHistory()->currentState());
        Console::showException(ex);
      }
    }
  }

  // ContextObserver
  void onActiveSiteChange(const Site& site) override {
    m_frame = site.frame();

    if (m_document == site.document())
      return;

    attachDocument(const_cast<Doc*>(site.document()));
  }

  // DocsObserver
  void onRemoveDocument(Doc* doc) override {
    if (m_document && m_document == doc)
      detachDocument();
  }

  // DocUndoObserver
  void onAddUndoState(DocUndo* history) override {
    ASSERT(history->currentState());
    Item* item = new Item(history->currentState());
    actions()->addChild(item);
    actions()->layout();
    view()->updateView();
    actions()->selectChild(item);
  }

  void onDeleteUndoState(DocUndo* history,
                         undo::UndoState* state) override {
    for (auto child : actions()->children()) {
      Item* item = static_cast<Item*>(child);
      if (item->state() == state) {
        actions()->removeChild(item);
        item->deferDelete();
        break;
      }
    }

    actions()->layout();
    view()->updateView();
  }

  void onCurrentUndoStateChange(DocUndo* history) override {
    selectState(history->currentState());
  }

  void onClearRedo(DocUndo* history) override {
    refillList(history);
  }

  void onTotalUndoSizeChange(DocUndo* history) override {
    updateTitle();
  }

  void attachDocument(Doc* document) {
    detachDocument();

    m_document = document;
    if (!document)
      return;

    DocUndo* history = m_document->undoHistory();
    history->add_observer(this);

    refillList(history);
    updateTitle();
  }

  void detachDocument() {
    if (!m_document)
      return;

    clearList();
    m_document->undoHistory()->remove_observer(this);
    m_document = nullptr;
    updateTitle();
  }

  void clearList() {
    ui::Widget* child;
    while ((child = actions()->firstChild()))
      delete child;

    actions()->layout();
    view()->updateView();
  }

  void refillList(DocUndo* history) {
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
    for (auto child : actions()->children()) {
      Item* item = static_cast<Item*>(child);
      if (item->state() == state) {
        actions()->selectChild(item);
        break;
      }
    }
  }

  void updateTitle() {
    if (!m_document)
      setText(m_title);
    else
      setTextf("%s (%s)",
               m_title.c_str(),
               base::get_pretty_memory_size(m_document->undoHistory()->totalUndoSize()).c_str());
  }

  Context* m_ctx;
  Doc* m_document;
  doc::frame_t m_frame;
  std::string m_title;
};

class UndoHistoryCommand : public Command {
public:
  UndoHistoryCommand();

protected:
  void onExecute(Context* ctx) override;
};

static UndoHistoryWindow* g_window = NULL;

UndoHistoryCommand::UndoHistoryCommand()
  : Command(CommandId::UndoHistory(), CmdUIOnlyFlag)
{
}

void UndoHistoryCommand::onExecute(Context* ctx)
{
  if (!g_window)
    g_window = new UndoHistoryWindow(ctx);

  if (g_window->isVisible())
    g_window->closeWindow(nullptr);
  else
    g_window->openWindow();
}

Command* CommandFactory::createUndoHistoryCommand()
{
  return new UndoHistoryCommand;
}

} // namespace app
