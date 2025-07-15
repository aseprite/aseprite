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
#include "gfx/rect.h"
#include "text/text_blob.h"
#include "ui/paint.h"

#include <string>

namespace app {

class FontInfo;

// Returns the exact bounds that are required to draw this TextBlob in
// the origin point (0, 0), i.e. the image size that will be required
// in render_text_blob().
gfx::RectF get_text_blob_required_bounds(const text::TextBlobRef& blob);

doc::ImageRef render_text_blob(const text::TextBlobRef& blob,
                               const gfx::RectF& textBounds,
                               const ui::Paint& paint);

doc::ImageRef render_text(const FontInfo& fontInfo,
                          const std::string& text,
                          const ui::Paint& paint);

} // namespace app

#endif
