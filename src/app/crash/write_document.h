// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CRASH_WRITE_DOCUMENT_H_INCLUDED
#define APP_CRASH_WRITE_DOCUMENT_H_INCLUDED
#pragma once

#include <string>

namespace doc {
  class CancelIO;
}

namespace app {
  class Doc;

  namespace crash {

    bool write_document(const std::string& dir, Doc* doc, doc::CancelIO* cancel);
    void delete_document_internals(Doc* doc);

  } // namespace crash
} // namespace app

#endif
