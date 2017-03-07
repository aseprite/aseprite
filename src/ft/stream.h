// Aseprite FreeType Wrapper
// Copyright (c) 2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef FT_STREAM_H_INCLUDED
#define FT_STREAM_H_INCLUDED
#pragma once

#include "ft/freetype_headers.h"

#include <string>

namespace ft {

FT_Stream open_stream(FT_Library lib,
                      const std::string& utf8Filename);

} // namespace ft

#endif
