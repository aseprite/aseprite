// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_CLI_CLI_OPEN_FILE_H_INCLUDED
#define APP_CLI_CLI_OPEN_FILE_H_INCLUDED
#pragma once

#include "doc/frame.h"
#include "gfx/rect.h"

#include <string>

namespace app {

  class Document;

  struct CliOpenFile {
    app::Document* document;
    std::string filename;
    std::string filenameFormat;
    std::string frameTagName;
    std::string importLayer;
    doc::frame_t frameFrom, frameTo;
    bool splitLayers;
    bool allLayers;
    bool listLayers;
    bool listTags;
    bool ignoreEmpty;
    bool trim;
    gfx::Rect crop;

    CliOpenFile() {
      document = nullptr;
      frameFrom = -1;
      frameTo = -1;
      splitLayers = false;
      allLayers = false;
      listLayers = false;
      listTags = false;
      ignoreEmpty = false;
      trim = false;
      crop = gfx::Rect();
    }

    bool hasFrameTagName() const {
      return (!frameTagName.empty());
    }

    bool hasFrameRange() const {
      return (frameFrom >= 0 && frameTo >= 0);
    }

  };

} // namespace app

#endif
