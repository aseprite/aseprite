// Aseprite Document Library
// Copyright (c) 2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_CONTEXT_H_INCLUDED
#define DOC_CONTEXT_H_INCLUDED
#pragma once

#include "base/compiler_specific.h"
#include "base/disable_copying.h"
#include "base/observable.h"
#include "doc/context_observer.h"
#include "doc/documents.h"
#include "doc/documents_observer.h"

namespace doc {
  class Command;
  class Document;
  class Settings;

  class Context : public base::Observable<ContextObserver>
                , public DocumentsObserver {
  public:
    Context();
    virtual ~Context();

    Settings* settings() const { return m_settings; }
    void setSettings(Settings* settings);

    const Documents& documents() const { return m_docs; }
    Documents& documents() { return m_docs; }

    Document* activeDocument() const;
    void setActiveDocument(Document* doc);

    // DocumentsObserver impl
    virtual void onAddDocument(Document* doc) OVERRIDE;
    virtual void onRemoveDocument(Document* doc) OVERRIDE;

  private:
    Settings* m_settings;
    Documents m_docs;
    Document* m_activeDoc;

    DISABLE_COPYING(Context);
  };

} // namespace doc

#endif
