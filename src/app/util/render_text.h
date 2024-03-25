// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UTIL_RENDER_TEXT_H_INCLUDED
#define APP_UTIL_RENDER_TEXT_H_INCLUDED
#pragma once

#include "doc/image_ref.h"
#include "gfx/color.h"
#include "text/font_mgr.h"

#include <string>

namespace app {

  class Color;
  class FontInfo;

  doc::ImageRef render_text(
    const text::FontMgrRef& fontMgr,
    const FontInfo& fontInfo,
    const std::string& text,
    gfx::Color color);

} // namespace app

#endif
