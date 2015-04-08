// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_CRASH_WRITE_DOCUMENT_H_INCLUDED
#define APP_CRASH_WRITE_DOCUMENT_H_INCLUDED
#pragma once

#include <string>

namespace app {
class Document;
namespace crash {

  void write_document(const std::string& dir, app::Document* doc);
  void delete_document_internals(app::Document* doc);

} // namespace crash
} // namespace app

#endif
