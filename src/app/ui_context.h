// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_CONTEXT_H_INCLUDED
#define APP_UI_CONTEXT_H_INCLUDED
#pragma once

#include "app/context.h"
#include "doc/documents_observer.h"

namespace app {
  class DocumentView;
  class Editor;

  typedef std::vector<DocumentView*> DocumentViews;

  class UIContext : public app::Context {
  public:
    static UIContext* instance() { return m_instance; }

    UIContext();
    virtual ~UIContext();

    bool isUIAvailable() const override;

    DocumentView* activeView() const;
    void setActiveView(DocumentView* documentView);
    void setActiveDocument(Document* document);

    DocumentView* getFirstDocumentView(doc::Document* document) const;
    DocumentViews getAllDocumentViews(doc::Document* document) const;

    // Returns the current editor. It can be null.
    Editor* activeEditor();

    // Returns the active editor for the given document, or creates a
    // new one if it's necessary.
    Editor* getEditorFor(Document* document);

  protected:
    void onAddDocument(doc::Document* doc) override;
    void onRemoveDocument(doc::Document* doc) override;
    void onGetActiveSite(doc::Site* site) const override;

  private:
    Document* m_lastSelectedDoc;
    DocumentView* m_lastSelectedView;
    static UIContext* m_instance;
  };

} // namespace app

#endif
