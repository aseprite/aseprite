/* Aseprite
 * Copyright (C) 2001-2014  David Capello
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
  };

  // These functions returns the new location of the "from" range or
  // throws an std::runtime_error() in case that the operation cannot
  // be done. (E.g. the background layer cannot be moved.)
  DocumentRange move_range(Document* doc, const DocumentRange& from, const DocumentRange& to, DocumentRangePlace place);
  DocumentRange copy_range(Document* doc, const DocumentRange& from, const DocumentRange& to, DocumentRangePlace place);

} // namespace app

#endif
