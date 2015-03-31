// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

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

    bool isUiAvailable() const override;

    DocumentView* activeView() const;
    void setActiveView(DocumentView* documentView);

    DocumentView* getFirstDocumentView(Document* document) const;

    // Returns the current editor. It can be null.
    Editor* activeEditor();

    // Returns the active editor for the given document, or creates a
    // new one if it's necessary.
    Editor* getEditorFor(Document* document);

  protected:
    virtual void onAddDocument(doc::Document* doc) override;
    virtual void onRemoveDocument(doc::Document* doc) override;
    virtual void onGetActiveLocation(DocumentLocation* location) const override;

  private:
    DocumentView* m_lastSelectedView;
    static UIContext* m_instance;
  };

} // namespace app

#endif
