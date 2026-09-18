// Aseprite Network Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef NET_HTTP_REQUEST_H_INCLUDED
#define NET_HTTP_REQUEST_H_INCLUDED
#pragma once

#include "base/disable_copying.h"

#include <cstdint>
#include <string>

namespace net {

class HttpHeaders;
class HttpRequestImpl;
class HttpResponse;

class HttpRequest {
public:
  enum class Method : uint8_t { GET, POST, PUT, PATCH, OPTIONS };

  explicit HttpRequest(const std::string& url, Method method = Method::GET);
  ~HttpRequest();

  void setHeaders(const HttpHeaders& headers);
  void setPostFields(const std::string& fields);
  bool send(HttpResponse& response, int timeoutMs = 0);
  void abort();

private:
  HttpRequestImpl* m_impl;

  DISABLE_COPYING(HttpRequest);
};

bool is_valid_url(std::string_view url);
std::string url_encode(std::string_view text);
std::string url_decode(std::string_view text);

} // namespace net

#endif
