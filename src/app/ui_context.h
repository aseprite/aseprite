// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_CONTEXT_H_INCLUDED
#define APP_UI_CONTEXT_H_INCLUDED
#pragma once

#include "app/context.h"
#include "app/docs_observer.h"

#include <vector>

namespace app {
  class DocView;
  class Editor;

  typedef std::vector<DocView*> DocViews;
  typedef std::vector<Editor*> Editors;

  class UIContext : public app::Context {
  public:
    static UIContext* instance() { return m_instance; }

    UIContext();
    virtual ~UIContext();

    bool isUIAvailable() const override;

    DocView* activeView() const;
    void setActiveView(DocView* documentView);

    DocView* getFirstDocView(Doc* document) const override;
    DocViews getAllDocViews(Doc* document) const;
    Editors getAllEditorsIncludingPreview(Doc* document) const;

    // Returns the current editor. It can be null.
    Editor* activeEditor();

    // Returns the active editor for the given document, or creates a
    // new one if it's necessary.
    Editor* getEditorFor(Doc* document);

    // Returns the list of closed docs in this session.
    const std::vector<Doc*>& closedDocs() const { return m_closedDocs; }
    void reopenClosedDoc(Doc* doc);

  protected:
    void onAddDocument(Doc* doc) override;
    void onRemoveDocument(Doc* doc) override;
    void onGetActiveSite(Site* site) const override;
    void onSetActiveDocument(Doc* doc) override;
    void onSetActiveLayer(doc::Layer* layer) override;
    void onSetActiveFrame(const doc::frame_t frame) override;
    void onCloseDocument(Doc* doc) override;

  private:
    DocView* m_lastSelectedView;
    std::vector<Doc*> m_closedDocs;

    static UIContext* m_instance;
  };

} // namespace app

#endif
