/* ASEPRITE
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef NET_HTTP_RESPONSE_H_INCLUDED
#define NET_HTTP_RESPONSE_H_INCLUDED

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
  void status(int status) { m_status = status; }

  // Writes data in the stream.
  void write(const char* data, size_t length);

private:
  int m_status;
  std::ostream* m_stream;

  DISABLE_COPYING(HttpResponse);
};

} // namespace net

#endif  // NET_HTTP_RESPONSE_H_INCLUDED
