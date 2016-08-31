// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_DOCUMENT_RANGE_OPS_H_INCLUDED
#define APP_DOCUMENT_RANGE_OPS_H_INCLUDED
#pragma once

#include <vector>

namespace app {
  class Document;
  class DocumentRange;

  enum DocumentRangePlace {
    kDocumentRangeBefore,
    kDocumentRangeAfter,
    kDocumentRangeFirstChild,
  };

  // These functions returns the new location of the "from" range or
  // throws an std::runtime_error() in case that the operation cannot
  // be done. (E.g. the background layer cannot be moved.)
  DocumentRange move_range(Document* doc, const DocumentRange& from, const DocumentRange& to, DocumentRangePlace place);
  DocumentRange copy_range(Document* doc, const DocumentRange& from, const DocumentRange& to, DocumentRangePlace place);

  void reverse_frames(Document* doc, const DocumentRange& range);

} // namespace app

#endif
