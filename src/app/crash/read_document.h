// Aseprite
// Copyright (c) 2018-present Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CRASH_READ_DOCUMENT_H_INCLUDED
#define APP_CRASH_READ_DOCUMENT_H_INCLUDED
#pragma once

#include "app/crash/document_info.h"
#include "app/crash/raw_images_as.h"
#include "base/task.h"
#include "doc/color_mode.h"
#include "doc/frame.h"

#include <string>

namespace app {
class Doc;
namespace crash {

bool read_document_info(const std::string& dir, DocumentInfo& info);
Doc* read_document(const std::string& dir, base::task_token* t);
Doc* read_document_with_raw_images(const std::string& dir, RawImagesAs as, base::task_token* t);

} // namespace crash
} // namespace app

#endif
