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
  class FileOpROI;

  struct CliOpenFile {
    app::Document* document;
    std::string filename;
    std::string filenameFormat;
    std::string frameTag;
    std::string importLayer;
    doc::frame_t fromFrame, toFrame;
    bool splitLayers;
    bool allLayers;
    bool listLayers;
    bool listTags;
    bool ignoreEmpty;
    bool trim;
    gfx::Rect crop;

    CliOpenFile();

    bool hasFrameTag() const {
      return (!frameTag.empty());
    }

    bool hasFrameRange() const {
      return (fromFrame >= 0 && toFrame >= 0);
    }

    FileOpROI roi() const;
  };

} // namespace app

#endif
