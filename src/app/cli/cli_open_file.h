// Aseprite
// Copyright (C) 2019-2022  Igara Studio S.A.
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
    Doc* document = nullptr;
    std::string filename;
    std::string filenameFormat;
    std::string tagnameFormat;
    std::string tag;
    std::string slice;
    std::vector<std::string> includeLayers;
    std::vector<std::string> excludeLayers;
    doc::frame_t fromFrame = -1;
    doc::frame_t toFrame = -1;
    bool splitLayers = false;
    bool splitTags = false;
    bool splitSlices = false;
    bool splitGrid = false;
    bool allLayers = false;
    bool listLayers = false;
    bool listTags = false;
    bool listSlices = false;
    bool ignoreEmpty = false;
    bool trim = false;
    bool trimByGrid = false;
    bool oneFrame = false;
    bool exportTileset = false;
    gfx::Rect crop;

    bool hasTag() const {
      return (!tag.empty());
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
