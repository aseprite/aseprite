// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CLI_CLI_OPEN_FILE_H_INCLUDED
#define APP_CLI_CLI_OPEN_FILE_H_INCLUDED
#pragma once

#include "doc/frame.h"
#include "gfx/rect.h"

#include <string>
#include <vector>

namespace app {

  class Document;
  class FileOpROI;

  struct CliOpenFile {
    app::Document* document;
    std::string filename;
    std::string filenameFormat;
    std::string frameTag;
    std::vector<std::string> importLayers;
    doc::frame_t fromFrame, toFrame;
    bool splitLayers;
    bool splitTags;
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
