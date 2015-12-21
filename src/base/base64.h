// Aseprite Base Library
// Copyright (c) 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_BASE64_H_INCLUDED
#define BASE_BASE64_H_INCLUDED
#pragma once

#include "base/buffer.h"

#include <string>

namespace base {

void encode_base64(const buffer& input, std::string& output);
void decode_base64(const std::string& input, buffer& output);

} // namespace base

#endif
