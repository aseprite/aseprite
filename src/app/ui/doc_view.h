// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_DOC_VIEW_H_INCLUDED
#define APP_UI_DOC_VIEW_H_INCLUDED
#pragma once

#include "app/doc_observer.h"
#include "app/ui/input_chain_element.h"
#include "app/ui/tabs.h"
#include "app/ui/workspace_view.h"
#include "ui/box.h"

namespace ui {
  class View;
}

namespace app {
  class Doc;
  class Editor;
  class Site;

  class DocViewPreviewDelegate {
  public:
    virtual ~DocViewPreviewDelegate() { }
    virtual void onScrollOtherEditor(Editor* editor) = 0;
    virtual void onDisposeOtherEditor(Editor* editor) = 0;
    virtual void onPreviewOtherEditor(Editor* editor) = 0;
  };

  class DocView : public ui::Box,
                  public TabView,
                  public app::DocObserver,
                  public WorkspaceView,
                  public app::InputChainElement {
  public:
    enum Type {
      Normal,
      Preview
    };

    DocView(Doc* document, Type type,
            DocViewPreviewDelegate* previewDelegate);
    ~DocView();

    Doc* document() const { return m_document; }
    Editor* editor() { return m_editor; }
    ui::View* viewWidget() const { return m_view; }
    void getSite(Site* site) const;

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
    bool onCloseView(Workspace* workspace, bool quitting) override;
    void onTabPopup(Workspace* workspace) override;
    InputChainElement* onGetInputChainElement() override { return this; }

    // DocObserver implementation
    void onGeneralUpdate(DocEvent& ev) override;
    void onSpritePixelsModified(DocEvent& ev) override;
    void onLayerMergedDown(DocEvent& ev) override;
    void onAddLayer(DocEvent& ev) override;
    void onAddFrame(DocEvent& ev) override;
    void onRemoveFrame(DocEvent& ev) override;
    void onAddCel(DocEvent& ev) override;
    void onRemoveCel(DocEvent& ev) override;
    void onTotalFramesChanged(DocEvent& ev) override;
    void onLayerRestacked(DocEvent& ev) override;

    // InputChainElement impl
    void onNewInputPriority(InputChainElement* element,
                            const ui::Message* msg) override;
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
    Doc* m_document;
    ui::View* m_view;
    DocViewPreviewDelegate* m_previewDelegate;
    Editor* m_editor;
  };

} // namespace app

#endif
