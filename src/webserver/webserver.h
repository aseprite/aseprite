/* Aseprite
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

#ifndef WEBSERVER_WEBSERVER_H_INCLUDED
#define WEBSERVER_WEBSERVER_H_INCLUDED

#include "base/disable_copying.h"
#include <string>
#include <iosfwd>

namespace webserver {

  class IRequest {
  public:
    virtual ~IRequest() { }
    virtual const char* getRequestMethod() = 0;
    virtual const char* getUri() = 0;
    virtual const char* getHttpVersion() = 0;
    virtual const char* getQueryString() = 0;
  };

  class IResponse {
  public:
    virtual ~IResponse() { }
    virtual void setStatusCode(int code) = 0;
    virtual void setContentType(const char* contentType) = 0;
    virtual std::ostream& getStream() = 0;
    virtual void sendFile(const char* path) = 0;
  };

  class IDelegate {
  public:
    virtual ~IDelegate() { }
    virtual void onProcessRequest(IRequest* request, IResponse* response) = 0;
  };

  class WebServer {
  public:
    class WebServerImpl;

    WebServer(IDelegate* delegate);
    ~WebServer();

    std::string getName() const;

  private:
    WebServerImpl* m_impl;

    DISABLE_COPYING(WebServer);
  };

} // namespace webserver

#endif
