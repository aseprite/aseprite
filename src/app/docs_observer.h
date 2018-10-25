// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (c) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_DOCS_OBSERVER_H_INCLUDED
#define APP_DOCS_OBSERVER_H_INCLUDED
#pragma once

namespace app {
  class Doc;

  class CreateDocArgs {
  public:
    CreateDocArgs() : m_doc(nullptr) { }
    Doc* document() { return m_doc; }
    void setDocument(Doc* doc) { m_doc = doc; }
  private:
    Doc* m_doc;
  };

  class DocsObserver {
  public:
    virtual ~DocsObserver() { }
    virtual void onAddDocument(Doc* doc) { }
    virtual void onRemoveDocument(Doc* doc) { }
  };

} // namespace app

#endif
