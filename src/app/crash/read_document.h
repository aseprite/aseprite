// Aseprite
// Copyright (c) 2018 Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CRASH_READ_DOCUMENT_H_INCLUDED
#define APP_CRASH_READ_DOCUMENT_H_INCLUDED
#pragma once

#include "app/crash/raw_images_as.h"
#include "doc/color_mode.h"
#include "doc/frame.h"

#include <string>

namespace app {
class Doc;
namespace crash {

  struct DocumentInfo {
    doc::ColorMode mode;
    int width;
    int height;
    doc::frame_t frames;
    std::string filename;

    DocumentInfo() :
      mode(doc::ColorMode::RGB),
      width(0),
      height(0),
      frames(0) {
    }
  };

  bool read_document_info(const std::string& dir, DocumentInfo& info);
  Doc* read_document(const std::string& dir);
  Doc* read_document_with_raw_images(const std::string& dir,
                                     RawImagesAs as);

} // namespace crash
} // namespace app

#endif
