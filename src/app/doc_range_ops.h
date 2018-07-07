// Aseprite
// Copyright (C) 2001-2018 David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_DOC_RANGE_OPS_H_INCLUDED
#define APP_DOC_RANGE_OPS_H_INCLUDED
#pragma once

#include "app/tags_handling.h"

#include <vector>

namespace app {
  class Document;
  class DocRange;

  enum DocRangePlace {
    kDocRangeBefore,
    kDocRangeAfter,
    kDocRangeFirstChild,
  };

  // These functions returns the new location of the "from" range or
  // throws an std::runtime_error() in case that the operation cannot
  // be done. (E.g. the background layer cannot be moved.)
  DocRange move_range(Document* doc,
                           const DocRange& from,
                           const DocRange& to,
                           const DocRangePlace place,
                           const TagsHandling tagsHandling = kDefaultTagsAdjustment);
  DocRange copy_range(Document* doc,
                           const DocRange& from,
                           const DocRange& to,
                           const DocRangePlace place,
                           const TagsHandling tagsHandling = kDefaultTagsAdjustment);

  void reverse_frames(Document* doc, const DocRange& range);

} // namespace app

#endif
