// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/with_document.h"

#include "app/cmd_exception.h"
#include "app/doc.h"

namespace app { namespace cmd {

WithDocument::WithDocument(Doc* doc) : m_docId(doc ? doc->id() : NullId)
{
}

Doc* WithDocument::document()
{
  Doc* doc = doc::get<Doc>(m_docId);
  if (!doc)
    throw CmdException(fmt::format("Invalid undo information: document ID {} not found", m_docId));
  return doc;
}

}} // namespace app::cmd
