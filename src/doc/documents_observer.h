// Aseprite Document Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_DOCUMENTS_OBSERVER_H_INCLUDED
#define DOC_DOCUMENTS_OBSERVER_H_INCLUDED
#pragma once

namespace doc {
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

} // namespace doc

#endif
