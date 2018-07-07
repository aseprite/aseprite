// Aseprite
// Copyright (c) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/docs.h"

#include "app/document.h"
#include "base/fs.h"
#include "base/mutex.h"
#include "base/unique_ptr.h"

#include <algorithm>

namespace app {

Docs::Docs(Context* ctx)
  : m_ctx(ctx)
{
  ASSERT(ctx != NULL);
}

Docs::~Docs()
{
  deleteAll();
}

Document* Docs::add(int width, int height, ColorMode mode, int ncolors)
{
  // Ask to observers to create the document (maybe a doc::Document or
  // a derived class).
  CreateDocumentArgs args;
  notify_observers(&DocsObserver::onCreateDocument, &args);
  if (!args.document())
    args.setDocument(new Document(nullptr));

  base::UniquePtr<Document> doc(args.document());
  doc->sprites().add(width, height, mode, ncolors);
  doc->setFilename("Sprite");
  doc->setContext(m_ctx); // Change the document context to add the doc in this collection

  return doc.release();
}

Document* Docs::add(Document* doc)
{
  ASSERT(doc != NULL);
  ASSERT(doc->id() != doc::NullId);

  if (doc->context() != m_ctx) {
    doc->setContext(m_ctx);
    ASSERT(std::find(begin(), end(), doc) != end());
    return doc;
  }

  m_docs.insert(begin(), doc);

  notify_observers(&DocsObserver::onAddDocument, doc);
  return doc;
}

void Docs::remove(Document* doc)
{
  iterator it = std::find(begin(), end(), doc);
  if (it == end())              // Already removed.
    return;

  m_docs.erase(it);

  notify_observers(&DocsObserver::onRemoveDocument, doc);

  doc->setContext(NULL);
}

void Docs::move(Document* doc, int index)
{
  iterator it = std::find(begin(), end(), doc);
  ASSERT(it != end());
  if (it != end())
    m_docs.erase(it);

  m_docs.insert(begin()+index, doc);
}

Document* Docs::getById(ObjectId id) const
{
  for (const auto& doc : *this) {
    if (doc->id() == id)
      return doc;
  }
  return NULL;
}

Document* Docs::getByName(const std::string& name) const
{
  for (const auto& doc : *this) {
    if (doc->name() == name)
      return doc;
  }
  return NULL;
}

Document* Docs::getByFileName(const std::string& filename) const
{
  std::string fn = base::normalize_path(filename);
  for (const auto& doc : *this) {
    if (doc->filename() == fn)
      return doc;
  }
  return NULL;
}

void Docs::deleteAll()
{
  while (!empty()) {
    ASSERT(m_ctx == back()->context());
    delete back();
  }
}

} // namespace app
