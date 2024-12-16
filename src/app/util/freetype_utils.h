// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UTIL_FREETYPE_UTILS_H_INCLUDED
#define APP_UTIL_FREETYPE_UTILS_H_INCLUDED
#pragma once

#include "doc/color.h"

#include <string>

namespace doc {
class Image;
}

namespace app {

doc::Image* render_text(const std::string& fontfile,
                        int fontsize,
                        const std::string& text,
                        doc::color_t color,
                        bool antialias);

} // namespace app

#endif
