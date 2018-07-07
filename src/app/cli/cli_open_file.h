// Aseprite
// Copyright (C) 2016-2017  David Capello
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

  class Doc;
  class FileOpROI;

  struct CliOpenFile {
    Doc* document;
    std::string filename;
    std::string filenameFormat;
    std::string frameTag;
    std::string slice;
    std::vector<std::string> includeLayers;
    std::vector<std::string> excludeLayers;
    doc::frame_t fromFrame, toFrame;
    bool splitLayers;
    bool splitTags;
    bool splitSlices;
    bool allLayers;
    bool listLayers;
    bool listTags;
    bool listSlices;
    bool ignoreEmpty;
    bool trim;
    bool oneFrame;
    gfx::Rect crop;

    CliOpenFile();

    bool hasFrameTag() const {
      return (!frameTag.empty());
    }

    bool hasSlice() const {
      return (!slice.empty());
    }

    bool hasFrameRange() const {
      return (fromFrame >= 0 && toFrame >= 0);
    }

    bool hasLayersFilter() const {
      return (!includeLayers.empty() ||
              !excludeLayers.empty());
    }

    FileOpROI roi() const;
  };

} // namespace app

#endif
