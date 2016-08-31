// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef WEBSERVER_WEBSERVER_H_INCLUDED
#define WEBSERVER_WEBSERVER_H_INCLUDED
#pragma once

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
