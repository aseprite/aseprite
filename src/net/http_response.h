// Aseprite Network Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef NET_HTTP_RESPONSE_H_INCLUDED
#define NET_HTTP_RESPONSE_H_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "http_headers.h"

namespace net {

class HttpResponse {
public:
  // Creates a response. The body of the response will be written in
  // the given "stream".
  HttpResponse(std::ostream* stream) : m_status(0), m_stream(stream) {}

  // Returns the HTTP status code.
  int status() const { return m_status; }
  void setStatus(int status) { m_status = status; }

  const HttpHeaders& headers() const { return m_headers; }
  void setHeaders(HttpHeaders&& headers) { m_headers = std::move(headers); }

  std::string error() const { return m_error; }
  void setError(const std::string& error) { m_error = error; }

  // Writes data in the stream.
  void write(const char* data, std::size_t length);

private:
  int m_status;
  HttpHeaders m_headers;
  std::string m_error;
  std::ostream* m_stream;

  DISABLE_COPYING(HttpResponse);
};

} // namespace net

#endif // NET_HTTP_RESPONSE_H_INCLUDED
