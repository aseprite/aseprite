// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_DOCUMENT_VIEW_H_INCLUDED
#define APP_UI_DOCUMENT_VIEW_H_INCLUDED
#pragma once

#include "app/ui/input_chain_element.h"
#include "app/ui/tabs.h"
#include "app/ui/workspace_view.h"
#include "doc/document_observer.h"
#include "ui/box.h"

namespace doc {
  class Site;
}

namespace ui {
  class View;
}

namespace app {
  class Document;
  class Editor;

  class DocumentView : public ui::Box
                     , public TabView
                     , public doc::DocumentObserver
                     , public WorkspaceView
                     , public app::InputChainElement {
  public:
    enum Type {
      Normal,
      Preview
    };

    DocumentView(Document* document, Type type);
    ~DocumentView();

    Document* getDocument() const { return m_document; }
    void getSite(doc::Site* site) const;
    Editor* getEditor() { return m_editor; }

    bool isPreview() { return m_type == Preview; }

    // TabView implementation
    std::string getTabText() override;
    TabIcon getTabIcon() override;

    // WorkspaceView implementation
    ui::Widget* getContentWidget() override { return this; }
    bool canCloneWorkspaceView() override { return true; }
    WorkspaceView* cloneWorkspaceView() override;
    void onWorkspaceViewSelected() override;
    void onClonedFrom(WorkspaceView* from) override;
    bool onCloseView(Workspace* workspace) override;
    void onTabPopup(Workspace* workspace) override;
    InputChainElement* onGetInputChainElement() override { return this; }

    // DocumentObserver implementation
    void onGeneralUpdate(doc::DocumentEvent& ev) override;
    void onSpritePixelsModified(doc::DocumentEvent& ev) override;
    void onLayerMergedDown(doc::DocumentEvent& ev) override;
    void onAddLayer(doc::DocumentEvent& ev) override;
    void onBeforeRemoveLayer(doc::DocumentEvent& ev) override;
    void onAddFrame(doc::DocumentEvent& ev) override;
    void onRemoveFrame(doc::DocumentEvent& ev) override;
    void onAddCel(doc::DocumentEvent& ev) override;
    void onRemoveCel(doc::DocumentEvent& ev) override;
    void onTotalFramesChanged(doc::DocumentEvent& ev) override;
    void onLayerRestacked(doc::DocumentEvent& ev) override;

    // InputChainElement impl
    void onNewInputPriority(InputChainElement* element) override;
    bool onCanCut(Context* ctx) override;
    bool onCanCopy(Context* ctx) override;
    bool onCanPaste(Context* ctx) override;
    bool onCanClear(Context* ctx) override;
    bool onCut(Context* ctx) override;
    bool onCopy(Context* ctx) override;
    bool onPaste(Context* ctx) override;
    bool onClear(Context* ctx) override;
    void onCancel(Context* ctx) override;

  protected:
    bool onProcessMessage(ui::Message* msg) override;

  private:
    Type m_type;
    Document* m_document;
    ui::View* m_view;
    Editor* m_editor;
  };

} // namespace app

#endif
