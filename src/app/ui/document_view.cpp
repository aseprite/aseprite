// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/document_view.h"

#include "app/app.h"
#include "app/app_menus.h"
#include "app/commands/commands.h"
#include "app/console.h"
#include "app/document_access.h"
#include "app/modules/editors.h"
#include "app/modules/palettes.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/editor_customization_delegate.h"
#include "app/ui/editor/editor_view.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/main_window.h"
#include "app/ui/preview_editor.h"
#include "app/ui/status_bar.h"
#include "app/ui/workspace.h"
#include "app/ui_context.h"
#include "base/path.h"
#include "doc/document_event.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "ui/accelerator.h"
#include "ui/alert.h"
#include "ui/menu.h"
#include "ui/message.h"
#include "ui/system.h"
#include "ui/view.h"

#include <typeinfo>

namespace app {

using namespace ui;

class AppEditor : public Editor,
                  public EditorObserver,
                  public EditorCustomizationDelegate {
public:
  AppEditor(Document* document) : Editor(document) {
    addObserver(this);
    setCustomizationDelegate(this);
  }

  ~AppEditor() {
    removeObserver(this);
    setCustomizationDelegate(NULL);
  }

  // EditorObserver implementation
  void dispose() override {
    App::instance()->getMainWindow()->getPreviewEditor()->updateUsingEditor(NULL);
  }

  void onScrollChanged(Editor* editor) override {
    if (current_editor == this)
      App::instance()->getMainWindow()->getPreviewEditor()->updateUsingEditor(this);
  }

  void onAfterFrameChanged(Editor* editor) override {
    App::instance()->getMainWindow()->getPreviewEditor()->updateUsingEditor(this);

    set_current_palette(editor->sprite()->palette(editor->frame()), true);
  }

  // EditorCustomizationDelegate implementation
  tools::Tool* getQuickTool(tools::Tool* currentTool) override {
    return KeyboardShortcuts::instance()
      ->getCurrentQuicktool(currentTool);
  }

  bool isCopySelectionKeyPressed() override {
    return isKeyActionPressed(KeyAction::CopySelection);
  }

  bool isSnapToGridKeyPressed() override {
    return isKeyActionPressed(KeyAction::SnapToGrid);
  }

  bool isAngleSnapKeyPressed() override {
    return isKeyActionPressed(KeyAction::AngleSnap);
  }

  bool isMaintainAspectRatioKeyPressed() override {
    return isKeyActionPressed(KeyAction::MaintainAspectRatio);
  }

  bool isLockAxisKeyPressed() override {
    return isKeyActionPressed(KeyAction::LockAxis);
  }

  bool isAddSelectionPressed() override {
    return isKeyActionPressed(KeyAction::AddSelection);
  }

  bool isSubtractSelectionPressed() override {
    return isKeyActionPressed(KeyAction::SubtractSelection);
  }

  bool isAutoSelectLayerPressed() override {
    return isKeyActionPressed(KeyAction::AutoSelectLayer);
  }

protected:
  bool onProcessMessage(Message* msg) override {
    switch (msg->type()) {

      case kKeyDownMessage:
      case kKeyUpMessage:
        if (static_cast<KeyMessage*>(msg)->repeat() == 0) {
          Key* lmb = KeyboardShortcuts::instance()->action(KeyAction::LeftMouseButton);
          Key* rmb = KeyboardShortcuts::instance()->action(KeyAction::RightMouseButton);

          // Convert action keys into mouse messages.
          if (lmb->isPressed(msg) || rmb->isPressed(msg)) {
            MouseMessage mouseMsg(
              (msg->type() == kKeyDownMessage ? kMouseDownMessage: kMouseUpMessage),
              (lmb->isPressed(msg) ? kButtonLeft: kButtonRight),
              ui::get_mouse_position());

            sendMessage(&mouseMsg);
            return true;
          }
        }
        break;
    }

    try {
      return Editor::onProcessMessage(msg);
    }
    catch (const std::exception& ex) {
      Console console;
      Console::showException(ex);
      console.printf("\nInternal details:\n"
        "- Message type: %d\n"
        "- Editor state: %s\n",
        msg->type(),
        getState() ? typeid(*getState().get()).name(): "None");
      return false;
    }
  }

private:
  bool isKeyActionPressed(KeyAction action) {
    if (Key* key = KeyboardShortcuts::instance()->action(action))
      return key->checkFromAllegroKeyArray();
    else
      return false;
  }

};

class PreviewEditor : public Editor,
                      public EditorObserver {
public:
  PreviewEditor(Document* document)
    : Editor(document, Editor::kShowOutside) // Don't show grid/mask in preview preview
  {
    addObserver(this);
  }

  ~PreviewEditor() {
    removeObserver(this);
  }

private:
  void onScrollChanged(Editor* editor) override {
    if (hasCapture()) {
      // TODO create a signal
      App::instance()->getMainWindow()->getPreviewEditor()->uncheckCenterButton();
    }
  }
};

DocumentView::DocumentView(Document* document, Type type)
  : Box(JI_VERTICAL)
  , m_type(type)
  , m_document(document)
  , m_view(new EditorView(type == Normal ? EditorView::CurrentEditorMode:
                                           EditorView::AlwaysSelected))
  , m_editor((type == Normal ?
      (Editor*)new AppEditor(document):
      (Editor*)new PreviewEditor(document)))
{
  addChild(m_view);

  m_view->attachToView(m_editor);
  m_view->setExpansive(true);

  m_editor->setDocumentView(this);
  m_document->addObserver(this);
}

DocumentView::~DocumentView()
{
  m_document->removeObserver(this);
  delete m_editor;
}

void DocumentView::getDocumentLocation(DocumentLocation* location) const
{
  m_editor->getDocumentLocation(location);
}

std::string DocumentView::getTabText()
{
  return m_document->name();
}

TabIcon DocumentView::getTabIcon()
{
  return TabIcon::NONE;
}

WorkspaceView* DocumentView::cloneWorkspaceView()
{
  return new DocumentView(m_document, Normal);
}

void DocumentView::onWorkspaceViewSelected()
{
  // Do nothing
}

void DocumentView::onClonedFrom(WorkspaceView* from)
{
  Editor* newEditor = getEditor();
  Editor* srcEditor = static_cast<DocumentView*>(from)->getEditor();

  newEditor->setLayer(srcEditor->layer());
  newEditor->setFrame(srcEditor->frame());
  newEditor->setZoom(srcEditor->zoom());

  View::getView(newEditor)
    ->setViewScroll(View::getView(srcEditor)->getViewScroll());
}

bool DocumentView::onCloseView(Workspace* workspace)
{
  // If there is another view for this document, just close the view.
  for (auto view : *workspace) {
    DocumentView* docView = dynamic_cast<DocumentView*>(view);
    if (docView && docView != this &&
        docView->getDocument() == getDocument()) {
      workspace->removeView(this);
      delete this;
      return true;
    }
  }

  UIContext* ctx = UIContext::instance();
  bool save_it;
  bool try_again = true;

  while (try_again) {
    // This flag indicates if we have to sabe the sprite before to destroy it
    save_it = false;
    {
      // see if the sprite has changes
      while (m_document->isModified()) {
        // ask what want to do the user with the changes in the sprite
        int ret = Alert::show("Warning<<Saving changes in:<<%s||&Save||Do&n't Save||&Cancel",
          m_document->name().c_str());

        if (ret == 1) {
          // "save": save the changes
          save_it = true;
          break;
        }
        else if (ret != 2) {
          // "cancel" or "ESC" */
          return false; // we back doing nothing
        }
        else {
          // "discard"
          break;
        }
      }
    }

    // Does we need to save the sprite?
    if (save_it) {
      ctx->setActiveView(this);
      ctx->updateFlags();

      Command* save_command =
        CommandsModule::instance()->getCommandByName(CommandId::SaveFile);
      ctx->executeCommand(save_command);

      try_again = true;
    }
    else
      try_again = false;
  }

  // Destroy the sprite (locking it as writer)
  DocumentDestroyer destroyer(
    static_cast<app::Context*>(m_document->context()), m_document, 250);

  StatusBar::instance()
    ->setStatusText(0, "Sprite '%s' closed.",
      m_document->name().c_str());

  destroyer.destroyDocument();

  // At this point the view is already destroyed

  return true;
}

void DocumentView::onTabPopup(Workspace* workspace)
{
  Menu* menu = AppMenus::instance()->getDocumentTabPopupMenu();
  if (!menu)
    return;

  UIContext* ctx = UIContext::instance();
  ctx->setActiveView(this);
  ctx->updateFlags();

  menu->showPopup(ui::get_mouse_position());
}

bool DocumentView::onProcessMessage(Message* msg)
{
  switch (msg->type()) {
    case kFocusEnterMessage:
      m_editor->requestFocus();
      break;
  }
  return Box::onProcessMessage(msg);
}

void DocumentView::onGeneralUpdate(doc::DocumentEvent& ev)
{
  if (m_editor->isVisible())
    m_editor->updateEditor();
}

void DocumentView::onSpritePixelsModified(doc::DocumentEvent& ev)
{
  if (m_editor->isVisible())
    m_editor->drawSpriteClipped(ev.region());
}

void DocumentView::onLayerMergedDown(doc::DocumentEvent& ev)
{
  m_editor->setLayer(ev.targetLayer());
}

void DocumentView::onAddLayer(doc::DocumentEvent& ev)
{
  if (current_editor == m_editor) {
    ASSERT(ev.layer() != NULL);
    m_editor->setLayer(ev.layer());
  }
}

void DocumentView::onBeforeRemoveLayer(doc::DocumentEvent& ev)
{
  Sprite* sprite = ev.sprite();
  Layer* layer = ev.layer();

  // If the layer that was removed is the selected one
  if (layer == m_editor->layer()) {
    LayerFolder* parent = layer->parent();
    Layer* layer_select = NULL;

    // Select previous layer, or next layer, or the parent (if it is
    // not the main layer of sprite set).
    if (layer->getPrevious())
      layer_select = layer->getPrevious();
    else if (layer->getNext())
      layer_select = layer->getNext();
    else if (parent != sprite->folder())
      layer_select = parent;

    m_editor->setLayer(layer_select);
  }
}

void DocumentView::onAddFrame(doc::DocumentEvent& ev)
{
  if (current_editor == m_editor)
    m_editor->setFrame(ev.frame());
  else if (m_editor->frame() > ev.frame())
    m_editor->setFrame(m_editor->frame()+1);
}

void DocumentView::onRemoveFrame(doc::DocumentEvent& ev)
{
  // Adjust current frame of all editors that are in a frame more
  // advanced that the removed one.
  if (m_editor->frame() > ev.frame()) {
    m_editor->setFrame(m_editor->frame()-1);
  }
  // If the editor was in the previous "last frame" (current value of
  // totalFrames()), we've to adjust it to the new last frame
  // (lastFrame())
  else if (m_editor->frame() >= m_editor->sprite()->totalFrames()) {
    m_editor->setFrame(m_editor->sprite()->lastFrame());
  }
}

void DocumentView::onTotalFramesChanged(doc::DocumentEvent& ev)
{
  if (m_editor->frame() >= m_editor->sprite()->totalFrames()) {
    m_editor->setFrame(m_editor->sprite()->lastFrame());
  }
}

void DocumentView::onLayerRestacked(doc::DocumentEvent& ev)
{
  m_editor->invalidate();
}

} // namespace app
