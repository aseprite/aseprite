// Aseprite
// Copyright (c) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_DOCUMENTS_OBSERVER_H_INCLUDED
#define APP_DOCUMENTS_OBSERVER_H_INCLUDED
#pragma once

namespace app {
  class Document;

  class CreateDocumentArgs {
  public:
    CreateDocumentArgs() : m_doc(nullptr) { }
    Document* document() { return m_doc; }
    void setDocument(Document* doc) { m_doc = doc; }
  private:
    Document* m_doc;
  };

  class DocumentsObserver {
  public:
    virtual ~DocumentsObserver() { }
    virtual void onCreateDocument(CreateDocumentArgs* args) { }
    virtual void onAddDocument(Document* doc) { }
    virtual void onRemoveDocument(Document* doc) { }
  };

} // namespace app

#endif
