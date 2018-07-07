// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_WITH_DOCUMENT_H_INCLUDED
#define APP_CMD_WITH_DOCUMENT_H_INCLUDED
#pragma once

#include "doc/object_id.h"

namespace app {
class Doc;
namespace cmd {

  class WithDocument {
  public:
    WithDocument(Doc* doc);
    Doc* document();

  private:
    doc::ObjectId m_docId;
  };

} // namespace cmd
} // namespace app

#endif
