// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_WEBSERVER_H_INCLUDED
#define APP_WEBSERVER_H_INCLUDED
#pragma once

#ifdef ENABLE_WEBSERVER

#include "webserver/webserver.h"

namespace app {

  class WebServer : public webserver::IDelegate {
  public:
    WebServer();
    ~WebServer();

    void start();

    // webserver::IDelegate implementation
    virtual void onProcessRequest(webserver::IRequest* request,
                                  webserver::IResponse* response) override;

  private:
    webserver::WebServer* m_webServer;
    std::string m_wwwpath;
  };

} // namespace app

#endif // ENABLE_WEBSERVER

#endif // APP_WEBSERVER_H_INCLUDED
