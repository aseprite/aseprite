// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_CLI_CLI_OPEN_FILE_H_INCLUDED
#define APP_CLI_CLI_OPEN_FILE_H_INCLUDED
#pragma once

#include "gfx/rect.h"

#include <string>

namespace app {

  class Document;

  struct CliOpenFile {
    app::Document* document;
    std::string filename;
    std::string filenameFormat;
    std::string frameTagName;
    std::string frameRange;
    std::string importLayer;
    bool splitLayers;
    bool allLayers;
    bool listLayers;
    bool listTags;
    bool ignoreEmpty;
    bool trim;
    gfx::Rect crop;

    CliOpenFile() {
      document = nullptr;
      splitLayers = false;
      allLayers = false;
      listLayers = false;
      listTags = false;
      ignoreEmpty = false;
      trim = false;
      crop = gfx::Rect();
    }

  };

} // namespace app

#endif
