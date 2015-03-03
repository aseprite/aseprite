// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef NET_HTTP_RESPONSE_H_INCLUDED
#define NET_HTTP_RESPONSE_H_INCLUDED
#pragma once

#include "base/disable_copying.h"

#include <iosfwd>

namespace net {

class HttpResponse
{
public:
  // Creates a response. The body of the response will be written in
  // the given "stream".
  HttpResponse(std::ostream* stream)
    : m_status(0)
    , m_stream(stream)
  { }

  // Returns the HTTP status code.
  int status() const { return m_status; }
  void setStatus(int status) { m_status = status; }

  // Writes data in the stream.
  void write(const char* data, size_t length);

private:
  int m_status;
  std::ostream* m_stream;

  DISABLE_COPYING(HttpResponse);
};

} // namespace net

#endif  // NET_HTTP_RESPONSE_H_INCLUDED
