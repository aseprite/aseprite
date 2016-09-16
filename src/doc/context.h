// Aseprite Document Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_CONTEXT_H_INCLUDED
#define DOC_CONTEXT_H_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "doc/context_observer.h"
#include "doc/documents.h"
#include "doc/documents_observer.h"
#include "obs/observable.h"

namespace doc {
  class Command;
  class Document;
  class Settings;

  class Context : public obs::observable<ContextObserver>
                , public DocumentsObserver {
  public:
    Context();
    virtual ~Context();

    const Documents& documents() const { return m_docs; }
    Documents& documents() { return m_docs; }

    Site activeSite() const;
    Document* activeDocument() const;

    void notifyActiveSiteChanged();

  protected:
    virtual void onGetActiveSite(Site* site) const;
    virtual void onAddDocument(Document* doc) override;
    virtual void onRemoveDocument(Document* doc) override;

  private:
    Settings* m_settings;
    Documents m_docs;
    Document* m_activeDoc;

    DISABLE_COPYING(Context);
  };

} // namespace doc

#endif
