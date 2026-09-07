// Aseprite Network Library
// Copyright (c) 2026-present Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef NET_HTTP_URL_H_INCLUDED
#define NET_HTTP_URL_H_INCLUDED
#pragma once

#include <string_view>

namespace net {

bool is_valid_url(std::string_view url);
std::string url_encode(std::string_view text);
std::string url_decode(std::string_view text);

} // namespace net

#endif
