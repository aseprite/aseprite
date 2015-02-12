// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_CMD_WITH_DOCUMENT_H_INCLUDED
#define APP_CMD_WITH_DOCUMENT_H_INCLUDED
#pragma once

#include "doc/object_id.h"

namespace app {
class Document;
namespace cmd {

  class WithDocument {
  public:
    WithDocument(app::Document* doc);
    app::Document* document();

  private:
    doc::ObjectId m_docId;
  };

} // namespace cmd
} // namespace app

#endif
