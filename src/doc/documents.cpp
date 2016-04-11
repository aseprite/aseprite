// Aseprite Document Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/documents.h"

#include "base/mutex.h"
#include "base/path.h"
#include "doc/document.h"

#include <algorithm>

namespace doc {

Documents::Documents(Context* ctx)
  : m_ctx(ctx)
{
  ASSERT(ctx != NULL);
}

Documents::~Documents()
{
  deleteAll();
}

Document* Documents::add(int width, int height, ColorMode mode, int ncolors)
{
  // Ask to observers to create the document (maybe a doc::Document or
  // a derived class).
  CreateDocumentArgs args;
  notifyObservers(&DocumentsObserver::onCreateDocument, &args);
  if (!args.document())
    args.setDocument(new Document());

  base::UniquePtr<Document> doc(args.document());
  doc->sprites().add(width, height, mode, ncolors);
  doc->setFilename("Sprite");
  doc->setContext(m_ctx); // Change the document context to add the doc in this collection

  return doc.release();
}

Document* Documents::add(Document* doc)
{
  ASSERT(doc != NULL);
  ASSERT(doc->id() != doc::NullId);

  if (doc->context() != m_ctx) {
    doc->setContext(m_ctx);
    ASSERT(std::find(begin(), end(), doc) != end());
    return doc;
  }

  m_docs.insert(begin(), doc);

  notifyObservers(&DocumentsObserver::onAddDocument, doc);
  return doc;
}

void Documents::remove(Document* doc)
{
  iterator it = std::find(begin(), end(), doc);
  if (it == end())              // Already removed.
    return;

  m_docs.erase(it);

  notifyObservers(&DocumentsObserver::onRemoveDocument, doc);

  doc->setContext(NULL);
}

void Documents::move(Document* doc, int index)
{
  iterator it = std::find(begin(), end(), doc);
  ASSERT(it != end());
  if (it != end())
    m_docs.erase(it);

  m_docs.insert(begin()+index, doc);
}

Document* Documents::getById(ObjectId id) const
{
  for (const auto& doc : *this) {
    if (doc->id() == id)
      return doc;
  }
  return NULL;
}

Document* Documents::getByName(const std::string& name) const
{
  for (const auto& doc : *this) {
    if (doc->name() == name)
      return doc;
  }
  return NULL;
}

Document* Documents::getByFileName(const std::string& filename) const
{
  std::string fn = base::normalize_path(filename);
  for (const auto& doc : *this) {
    if (doc->filename() == fn)
      return doc;
  }
  return NULL;
}

void Documents::deleteAll()
{
  while (!empty()) {
    ASSERT(m_ctx == back()->context());
    delete back();
  }
}

} // namespace doc
