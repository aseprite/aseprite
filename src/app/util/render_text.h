// Aseprite
// Copyright (C) 2024-2025  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UTIL_RENDER_TEXT_H_INCLUDED
#define APP_UTIL_RENDER_TEXT_H_INCLUDED
#pragma once

#include "doc/image_ref.h"
#include "gfx/color.h"
#include "text/text_blob.h"

#include <string>

namespace app {

class Color;
class FontInfo;
namespace skin {
class SkinTheme;
}

// Returns the exact bounds that are required to draw this TextBlob,
// i.e. the image size that will be required in render_text_blob().
gfx::Size get_text_blob_required_size(const text::TextBlobRef& blob);

doc::ImageRef render_text_blob(const text::TextBlobRef& blob, gfx::Color color);

doc::ImageRef render_text(const FontInfo& fontInfo, const std::string& text, gfx::Color color);

} // namespace app

#endif
