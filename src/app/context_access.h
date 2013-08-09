/* Aseprite
 * Copyright (C) 2001-2012  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef APP_CONTEXT_ACCESS_H_INCLUDED
#define APP_CONTEXT_ACCESS_H_INCLUDED

#include "app/document_access.h"
#include "app/document_location.h"

namespace app {

  class Context;

  template<typename DocumentAccessT>
  class ContextAccess {
  public:
    const Context* context() const { return m_context; }
    const DocumentLocation* location() const { return &m_location; }
    const DocumentAccessT& document() const { return m_document; }
    const Sprite* sprite() const { return m_location.sprite(); }
    const Layer* layer() const { return m_location.layer(); }
    FrameNumber frame() const { return m_location.frame(); }
    const Cel* cel() const { return m_location.cel(); }

    // You cannot change the location directly from a writable ContextAccess anyway.
    const DocumentLocation* location() { return &m_location; }

    Context* context() { return const_cast<Context*>(m_context); }
    DocumentAccessT& document() { return m_document; }
    Sprite* sprite() { return m_location.sprite(); }
    Layer* layer() { return m_location.layer(); }
    Cel* cel() { return m_location.cel(); }

    Image* image(int* x = NULL, int* y = NULL, int* opacity = NULL) const {
      return m_location.image(x, y, opacity); 
    }

  protected:
    ContextAccess(const Context* context)
      : m_context(context)
      , m_document(context->getActiveDocument())
      , m_location(context->getActiveLocation())
    {
    }

    template<typename DocumentReaderT>
    ContextAccess(const Context* context, const DocumentReaderT& documentReader)
      : m_context(context)
      , m_document(documentReader)
      , m_location(context->getActiveLocation())
    {
    }

  private:
    const Context* m_context;
    DocumentAccessT m_document;
    DocumentLocation m_location;
  };

  // You can use this class to access to the given context to read the
  // active document.
  class ContextReader : public ContextAccess<DocumentReader> {
  public:
    ContextReader(const Context* context)
      : ContextAccess(context) {
    }
  };

  // You can use this class to access to the given context to write the
  // active document.
  class ContextWriter : public ContextAccess<DocumentWriter> {
  public:
    ContextWriter(const Context* context)
      : ContextAccess(context) {
    }

    ContextWriter(const ContextReader& reader)
      : ContextAccess(reader.context(), reader.document()) {
    }
  };

} // namespace app

#endif
