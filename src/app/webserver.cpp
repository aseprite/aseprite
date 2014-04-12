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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef ENABLE_WEBSERVER

#include "app/webserver.h"

#include "base/fs.h"
#include "base/path.h"
#include "app/resource_finder.h"
#include "webserver/webserver.h"

#include <fstream>

#define API_VERSION 1

namespace app {

WebServer::WebServer()
  : m_webServer(NULL)
{
  ResourceFinder rf;
  rf.includeDataDir("www");

  while (rf.next()) {
    if (base::directory_exists(rf.filename())) {
      m_wwwpath = rf.filename();
      break;
    }
  }
}

WebServer::~WebServer()
{
  delete m_webServer;
}

void WebServer::start()
{
  m_webServer = new webserver::WebServer(this);
}

void WebServer::onProcessRequest(webserver::IRequest* request,
                                 webserver::IResponse* response)
{
  std::string uri = request->getUri();
  if (!uri.empty() && uri[uri.size()-1] == '/')
    uri.erase(uri.size()-1);

  if (uri == "/version") {
    response->setContentType("text/plain");
    response->getStream() << "{\"package\":\"" << PACKAGE "\","
                          << "\"version\":\"" << VERSION << "\","
                          << "\"webserver\":\"" << m_webServer->getName() << "\","
                          << "\"api\":\"" << API_VERSION << "\"}";
  }
  else {
    if (uri == "/" || uri.empty())
      uri = "/index.html";

    std::string fn = base::join_path(m_wwwpath, uri);
    if (base::file_exists(fn)) {
      response->sendFile(fn.c_str());
    }
    else {
      response->setStatusCode(404);
      response->getStream() << "Not found\n"
                            << "URI = " << uri << "\n"
                            << "Local file = " << fn;
    }
  }
}

}

#endif // ENABLE_WEBSERVER
