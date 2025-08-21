// Aseprite
// Copyright (C) 2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_DOC_API_DND_HELPER_H_INCLUDED
#define APP_DOC_API_DND_HELPER_H_INCLUDED
#pragma once

#include "app/doc.h"

namespace app { namespace docapi {

enum class InsertionPoint {
  BeforeLayer,
  AfterLayer,
};

enum class DroppedOn {
  Unspecified,
  Frame,
  Layer,
  Cel,
};

// Used by the drag & drop helper API as a provider of the documents that
// were dragged and then dropped in some Drag&Drop aware widget.
class DocProvider {
public:
  virtual ~DocProvider() = default;
  // Returns the next document from the underlying provider's implementation,
  // returns null if there is no more documents to provide.
  virtual std::unique_ptr<Doc> nextDoc() = 0;
  // Returns the number of docs that were not provided yet.
  virtual int pendingDocs() = 0;
};

}} // namespace app::docapi

#endif
