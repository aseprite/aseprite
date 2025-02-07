// Aseprite
// Copyright (C) 2020-2022  Igara Studio S.A.
// Copyright (C) 2015-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/cmd.h"
#include "app/cmd_transaction.h"
#include "app/commands/command.h"
#include "app/console.h"
#include "app/context.h"
#include "app/context_observer.h"
#include "app/doc.h"
#include "app/doc_access.h"
#include "app/doc_undo.h"
#include "app/doc_undo_observer.h"
#include "app/docs_observer.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/site.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/workspace.h"
#include "base/mem_utils.h"
#include "fmt/format.h"
#include "ui/init_theme_event.h"
#include "ui/listitem.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"
#include "ui/view.h"
#include "undo/undo_state.h"

#include "undo_history.xml.h"

namespace app {

using namespace app::skin;

class UndoHistoryWindow : public app::gen::UndoHistory,
                          public ContextObserver,
                          public DocsObserver,
                          public DocUndoObserver {
public:
  class ActionsList final : public ui::Widget {
  public:
    ActionsList(UndoHistoryWindow* window) : m_window(window)
    {
      setFocusStop(true);
      initTheme();
    }

    void setUndoHistory(Doc* doc, DocUndo* history)
    {
      m_doc = doc;
      m_undoHistory = history;

      updateSavedState();

      invalidate();
    }

    void selectState(const undo::UndoState* state)
    {
      auto view = ui::View::getView(this);
      if (!view)
        return;

      invalidate();

      gfx::Point scroll = view->viewScroll();
      if (state) {
        const gfx::Rect vp = view->viewportBounds();
        const gfx::Rect bounds = this->bounds();

        gfx::Rect itemBounds(bounds.x, bounds.y, bounds.w, m_itemHeight);

        // Jump first "Initial State"
        itemBounds.y += itemBounds.h;

        const undo::UndoState* s = m_undoHistory->firstState();
        while (s) {
          if (s == state)
            break;
          itemBounds.y += itemBounds.h;
          s = s->next();
        }

        if (itemBounds.y < vp.y)
          scroll.y = itemBounds.y - bounds.y;
        else if (itemBounds.y > vp.y + vp.h - itemBounds.h)
          scroll.y = (itemBounds.y - bounds.y - vp.h + itemBounds.h);
      }
      else {
        scroll = gfx::Point(0, 0);
      }

      view->setViewScroll(scroll);
    }

    void updateSavedState()
    {
      const auto oldIsAssociatedToFile = m_isAssociatedToFile;
      const auto oldSavedState = m_savedState;

      m_isAssociatedToFile = (m_doc && m_undoHistory ? m_doc->isAssociatedToFile() &&
                                                         !m_undoHistory->isSavedStateIsLost() :
                                                       false);

      // Get the state where this sprite was saved.
      m_savedState = nullptr;
      if (m_undoHistory && m_isAssociatedToFile)
        m_savedState = m_undoHistory->savedState();

      if (oldIsAssociatedToFile != m_isAssociatedToFile || oldSavedState != m_savedState) {
        invalidate();
      }
    }

    obs::signal<void(const undo::UndoState*)> Change;

  protected:
    void onInitTheme(ui::InitThemeEvent& ev) override
    {
      Widget::onInitTheme(ev);
      auto theme = SkinTheme::get(this);
      m_itemHeight = textHeight() + theme->calcBorder(this, theme->styles.listItem()).height();
    }

    bool onProcessMessage(ui::Message* msg) override
    {
      switch (msg->type()) {
        case ui::kMouseDownMessage: captureMouse(); [[fallthrough]];

        case ui::kMouseMoveMessage:
          if (hasCapture() && m_undoHistory) {
            auto mouseMsg = static_cast<ui::MouseMessage*>(msg);
            const gfx::Rect bounds = this->bounds();

            // Mouse position in client coordinates
            const gfx::Point mousePos = mouseMsg->position();
            gfx::Rect itemBounds(bounds.x, bounds.y, bounds.w, m_itemHeight);

            // First state
            if (itemBounds.contains(mousePos)) {
              Change(nullptr);
              break;
            }
            itemBounds.y += itemBounds.h;

            const undo::UndoState* state = m_undoHistory->firstState();
            while (state) {
              if (itemBounds.contains(mousePos)) {
                Change(state);
                break;
              }
              itemBounds.y += itemBounds.h;
              state = state->next();
            }
          }
          break;

        case ui::kMouseUpMessage:    releaseMouse(); break;

        case ui::kMouseWheelMessage: {
          auto view = ui::View::getView(this);
          if (view) {
            auto mouseMsg = static_cast<ui::MouseMessage*>(msg);
            gfx::Point scroll = view->viewScroll();

            if (mouseMsg->preciseWheel())
              scroll += mouseMsg->wheelDelta();
            else
              scroll += mouseMsg->wheelDelta() * 3 * (m_itemHeight + 4 * ui::guiscale());

            view->setViewScroll(scroll);
          }
          break;
        }

        case ui::kKeyDownMessage:
          if (hasFocus() && m_undoHistory) {
            const undo::UndoState* current = m_undoHistory->currentState();
            const undo::UndoState* select = current;
            auto view = ui::View::getView(this);
            const gfx::Rect vp = view->viewportBounds();
            ui::KeyMessage* keymsg = static_cast<ui::KeyMessage*>(msg);
            ui::KeyScancode scancode = keymsg->scancode();

            if (keymsg->onlyCmdPressed()) {
              if (scancode == ui::kKeyUp)
                scancode = ui::kKeyHome;
              if (scancode == ui::kKeyDown)
                scancode = ui::kKeyEnd;
            }

            switch (scancode) {
              case ui::kKeyUp:
                if (select)
                  select = select->prev();
                else
                  select = m_undoHistory->lastState();
                break;
              case ui::kKeyDown:
                if (select)
                  select = select->next();
                else
                  select = m_undoHistory->firstState();
                break;
              case ui::kKeyHome: select = nullptr; break;
              case ui::kKeyEnd:  select = m_undoHistory->lastState(); break;
              case ui::kKeyPageUp:
                for (int i = 0; select && i < vp.h / m_itemHeight; ++i)
                  select = select->prev();
                break;
              case ui::kKeyPageDown: {
                int i = 0;
                if (!select) {
                  select = m_undoHistory->firstState();
                  i = 1;
                }
                for (; select && i < vp.h / m_itemHeight; ++i)
                  select = select->next();
                break;
              }
              default: return Widget::onProcessMessage(msg);
            }

            if (select != current)
              Change(select);
            return true;
          }
          break;
      }
      return Widget::onProcessMessage(msg);
    }

    void onPaint(ui::PaintEvent& ev) override
    {
      ui::Graphics* g = ev.graphics();
      auto theme = SkinTheme::get(this);
      gfx::Rect bounds = clientBounds();

      g->fillRect(theme->colors.background(), bounds);

      if (!m_undoHistory)
        return;

      const undo::UndoState* currentState = m_undoHistory->currentState();
      gfx::Rect itemBounds(bounds.x, bounds.y, bounds.w, m_itemHeight);

      // First state
      {
        const bool selected = (currentState == nullptr);
        paintItem(g, theme, nullptr, itemBounds, selected);
        itemBounds.y += itemBounds.h;
      }

      const undo::UndoState* state = m_undoHistory->firstState();
      while (state) {
        const bool selected = (state == currentState);
        paintItem(g, theme, state, itemBounds, selected);
        itemBounds.y += itemBounds.h;
        state = state->next();
      }
    }

    void onSizeHint(ui::SizeHintEvent& ev) override
    {
      if (m_window->m_nitems == 0) {
        int size = 0;
        if (m_undoHistory) {
          ++size;
          const undo::UndoState* state = m_undoHistory->firstState();
          while (state) {
            ++size;
            state = state->next();
          }
        }
        m_window->m_nitems = size;
      }
      ev.setSizeHint(gfx::Size(1, m_itemHeight * m_window->m_nitems));
    }

  private:
    void paintItem(ui::Graphics* g,
                   SkinTheme* theme,
                   const undo::UndoState* state,
                   const gfx::Rect& itemBounds,
                   const bool selected)
    {
      const std::string itemText =
        (state ? static_cast<Cmd*>(state->cmd())->label()
#if _DEBUG
                   + std::string(" ") +
                   base::get_pretty_memory_size(static_cast<Cmd*>(state->cmd())->memSize())
#endif
                 :
                 std::string("Initial State"));

      if ((g->getClipBounds() & itemBounds).isEmpty())
        return;

      auto style = theme->styles.listItem();
      if (m_isAssociatedToFile && m_savedState == state) {
        style = theme->styles.undoSavedItem();
      }

      ui::PaintWidgetPartInfo info;
      info.text = &itemText;
      info.styleFlags = (selected ? ui::Style::Layer::kSelected : 0);
      theme->paintWidgetPart(g, style, itemBounds, info);
    }

    UndoHistoryWindow* m_window;
    Doc* m_doc = nullptr;
    DocUndo* m_undoHistory = nullptr;
    bool m_isAssociatedToFile = false;
    const undo::UndoState* m_savedState = nullptr;
    int m_itemHeight;
  };

  UndoHistoryWindow(Context* ctx) : m_ctx(ctx), m_doc(nullptr), m_actions(this)
  {
    m_title = text();
    m_actions.Change.connect(&UndoHistoryWindow::onChangeAction, this);
    view()->attachToView(&m_actions);
  }

private:
  bool onProcessMessage(ui::Message* msg) override
  {
    switch (msg->type()) {
      case ui::kOpenMessage:
        load_window_pos(this, "UndoHistory");

        m_ctx->add_observer(this);
        m_ctx->documents().add_observer(this);
        if (m_ctx->activeDocument()) {
          m_frame = m_ctx->activeSite().frame();

          attachDocument(m_ctx->activeDocument());
        }

        view()->invalidate();
        break;

      case ui::kCloseMessage:
        save_window_pos(this, "UndoHistory");

        if (m_doc)
          detachDocument();
        m_ctx->documents().remove_observer(this);
        m_ctx->remove_observer(this);
        break;
    }
    return app::gen::UndoHistory::onProcessMessage(msg);
  }

  void onChangeAction(const undo::UndoState* state)
  {
    if (m_doc && m_doc->undoHistory()->currentState() != state) {
      try {
        DocWriter writer(m_doc, 100);
        m_doc->undoHistory()->moveToState(state);
        m_doc->generateMaskBoundaries();

        // TODO this should be an observer of the current document palette
        set_current_palette(m_doc->sprite()->palette(m_frame), false);

        m_doc->notifyGeneralUpdate();
        m_actions.invalidate();
      }
      catch (const std::exception& ex) {
        selectCurrentState();
        Console::showException(ex);
      }
    }
  }

  // ContextObserver
  void onActiveSiteChange(const Site& site) override
  {
    m_frame = site.frame();

    if (m_doc == site.document())
      return;

    attachDocument(const_cast<Doc*>(site.document()));
  }

  // DocsObserver
  void onRemoveDocument(Doc* doc) override
  {
    if (m_doc && m_doc == doc)
      detachDocument();
  }

  // DocUndoObserver
  void onAddUndoState(DocUndo* history) override
  {
    ASSERT(history->currentState());

    ++m_nitems;

    m_actions.updateSavedState();
    m_actions.invalidate();
    view()->updateView();

    selectCurrentState();
  }

  void onDeleteUndoState(DocUndo* history, undo::UndoState* state) override
  {
    m_actions.updateSavedState();
    --m_nitems;
  }

  void onCurrentUndoStateChange(DocUndo* history) override
  {
    selectCurrentState();

    // TODO DocView should be an DocUndoObserver and update its state automatically
    App::instance()->workspace()->updateTabs();
  }

  void onClearRedo(DocUndo* history) override { setUndoHistory(history); }

  void onTotalUndoSizeChange(DocUndo* history) override { updateTitle(); }

  void onNewSavedState(DocUndo* history) override { m_actions.updateSavedState(); }

  void attachDocument(Doc* doc)
  {
    if (m_doc == doc)
      return;

    detachDocument();
    m_doc = doc;
    if (!doc)
      return;

    DocUndo* history = m_doc->undoHistory();
    history->add_observer(this);

    setUndoHistory(history);
    updateTitle();
  }

  void detachDocument()
  {
    if (!m_doc)
      return;

    m_doc->undoHistory()->remove_observer(this);
    m_doc = nullptr;

    setUndoHistory(nullptr);
    updateTitle();
  }

  void setUndoHistory(DocUndo* history)
  {
    m_nitems = 0;
    m_actions.setUndoHistory(m_doc, history);
    view()->updateView();

    if (history)
      selectCurrentState();
  }

  void selectCurrentState() { m_actions.selectState(m_doc->undoHistory()->currentState()); }

  void updateTitle()
  {
    if (!m_doc)
      setText(m_title);
    else {
      setText(fmt::format("{} ({})",
                          m_title,
                          base::get_pretty_memory_size(m_doc->undoHistory()->totalUndoSize())));
    }
  }

  Context* m_ctx;
  Doc* m_doc;
  doc::frame_t m_frame;
  std::string m_title;
  ActionsList m_actions;
  int m_nitems = 0;
};

class UndoHistoryCommand : public Command {
public:
  UndoHistoryCommand();

protected:
  void onExecute(Context* ctx) override;
};

static UndoHistoryWindow* g_window = NULL;

UndoHistoryCommand::UndoHistoryCommand() : Command(CommandId::UndoHistory(), CmdUIOnlyFlag)
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
