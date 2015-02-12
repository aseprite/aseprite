// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef NET_HTTP_REQUEST_H_INCLUDED
#define NET_HTTP_REQUEST_H_INCLUDED
#pragma once

#include "base/disable_copying.h"

#include <string>

namespace net {

class HttpHeaders;
class HttpRequestImpl;
class HttpResponse;

class HttpRequest
{
public:
  HttpRequest(const std::string& url);
  ~HttpRequest();

  void setHeaders(const HttpHeaders& headers);
  void send(HttpResponse& response);
  void abort();

private:
  HttpRequestImpl* m_impl;

  DISABLE_COPYING(HttpRequest);
};

} // namespace net

#endif
