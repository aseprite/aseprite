// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_FILE_FILE_DATA_H_INCLUDED
#define APP_FILE_FILE_DATA_H_INCLUDED
#pragma once

#include "app/color.h"
#include <string>

namespace doc {
  class Document;
}

namespace app {

void load_aseprite_data_file(const std::string& dataFilename,
                             doc::Document* doc,
                             app::Color& defaultSliceColor);
#ifdef ENABLE_SAVE
void save_aseprite_data_file(const std::string& dataFilename, const doc::Document* doc);
#endif

} // namespace app

#endif
