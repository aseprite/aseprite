// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/context.h"

#include <algorithm>

namespace doc {

Context::Context()
  : m_settings(NULL)
  , m_docs(this)
  , m_activeDoc(NULL)
{
  m_docs.addObserver(this);
}

Context::~Context()
{
  setActiveDocument(NULL);
  m_docs.removeObserver(this);
}

void Context::setSettings(Settings* settings)
{
  m_settings = settings;
}

Document* Context::activeDocument() const
{
  return m_activeDoc;
}

void Context::setActiveDocument(Document* doc)
{
  m_activeDoc = doc;
  notifyObservers(&ContextObserver::onSetActiveDocument, doc);
}

void Context::onAddDocument(Document* doc)
{
  m_activeDoc = doc;
}

void Context::onRemoveDocument(Document* doc)
{
  if (m_activeDoc == doc)
    setActiveDocument(!m_docs.empty() ? m_docs.back() : NULL);
}

} // namespace doc
