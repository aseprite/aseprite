// Aseprite
// Copyright (c) 2026-present Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CRASH_DOCUMENT_INFO_H_INCLUDED
#define APP_CRASH_DOCUMENT_INFO_H_INCLUDED
#pragma once

#include "doc/color_mode.h"
#include "doc/frame.h"
#include "doc/object_id.h"

#include <string>

namespace app {
class Doc;
namespace crash {

struct DocumentInfo {
  doc::ObjectId docId;
  doc::ColorMode mode;
  int width;
  int height;
  doc::frame_t frames;
  std::string filename;

  DocumentInfo() : mode(doc::ColorMode::RGB), width(0), height(0), frames(0) {}
  explicit DocumentInfo(const Doc* doc);

  std::string toString(bool withFullPath) const;
};

} // namespace crash
} // namespace app

#endif
