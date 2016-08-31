// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/with_document.h"

#include "app/document.h"

namespace app {
namespace cmd {

WithDocument::WithDocument(app::Document* doc)
  : m_docId(doc->id())
{
}

app::Document* WithDocument::document()
{
  return doc::get<app::Document>(m_docId);
}

} // namespace cmd
} // namespace app
