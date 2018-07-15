// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CONTEXT_ACCESS_H_INCLUDED
#define APP_CONTEXT_ACCESS_H_INCLUDED
#pragma once

#include "app/doc_access.h"
#include "app/site.h"

namespace app {

  class Context;

  template<typename DocumentAccessT>
  class ContextAccess {
  public:
    const Context* context() const { return m_context; }
    const Site* site() const { return &m_site; }
    const DocumentAccessT& document() const { return m_document; }
    const Sprite* sprite() const { return m_site.sprite(); }
    const Layer* layer() const { return m_site.layer(); }
    frame_t frame() const { return m_site.frame(); }
    const Cel* cel() const { return m_site.cel(); }

    // You cannot change the site directly from a writable ContextAccess anyway.
    const Site* site() { return &m_site; }

    Context* context() { return const_cast<Context*>(m_context); }
    DocumentAccessT& document() { return m_document; }
    Sprite* sprite() { return m_site.sprite(); }
    Layer* layer() { return m_site.layer(); }
    Cel* cel() { return m_site.cel(); }

    Image* image(int* x = NULL, int* y = NULL, int* opacity = NULL) const {
      return m_site.image(x, y, opacity);
    }

    Palette* palette() const {
      return m_site.palette();
    }

  protected:
    ContextAccess(const Context* context, int timeout)
      : m_context(context)
      , m_document(context->activeDocument(), timeout)
      , m_site(context->activeSite())
    {
    }

    template<typename DocReaderT>
    ContextAccess(const Context* context, const DocReaderT& docReader, int timeout)
      : m_context(context)
      , m_document(docReader, timeout)
      , m_site(context->activeSite())
    {
    }

  private:
    const Context* m_context;
    DocumentAccessT m_document;
    Site m_site;
  };

  // You can use this class to access to the given context to read the
  // active document.
  class ContextReader : public ContextAccess<DocReader> {
  public:
    ContextReader(const Context* context, int timeout = 0)
      : ContextAccess<DocReader>(context, timeout) {
    }
  };

  // You can use this class to access to the given context to write the
  // active document.
  class ContextWriter : public ContextAccess<DocWriter> {
  public:
    ContextWriter(const Context* context, int timeout = 0)
      : ContextAccess<DocWriter>(context, timeout) {
    }

    ContextWriter(const ContextReader& reader, int timeout = 0)
      : ContextAccess<DocWriter>(reader.context(), reader.document(), timeout) {
    }
  };

} // namespace app

#endif
