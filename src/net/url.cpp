// Aseprite Network Library
// Copyright (c) 2026-present Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <curl/curl.h>

#include "net/url.h"

namespace net {

bool is_valid_url(const std::string_view url)
{
  CURLU* h = curl_url();
  if (!h)
    return false;
  const auto rc = curl_url_set(h, CURLUPART_URL, url.data(), 0);
  curl_url_cleanup(h);
  return rc == CURLUE_OK;
}

std::string url_encode(const std::string_view text)
{
  std::string result;
  if (auto* escaped = curl_easy_escape(nullptr, text.data(), text.length())) {
    result = escaped;
    curl_free(escaped);
  }
  return result;
}

std::string url_decode(const std::string_view text)
{
  std::string result;
  int len;
  if (auto* unescaped = curl_easy_unescape(nullptr, text.data(), text.length(), &len)) {
    result = std::string(unescaped, len);
    curl_free(unescaped);
  }
  return result;
}

} // namespace net
