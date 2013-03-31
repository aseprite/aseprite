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

#include "config.h"

#include "webserver/webserver.h"

#include "base/bind.h"
#include "mongoose.h"

#include <sstream>

namespace webserver {

static int begin_request_handler(mg_connection* conn);

class WebServer::WebServerImpl
{
public:
  WebServerImpl() {
    const char* options[] = { "listening_ports", "10453", NULL};

    memset(&m_callbacks, 0, sizeof(m_callbacks));
    m_callbacks.begin_request = &begin_request_handler;

    m_context = mg_start(&m_callbacks, (void*)this, options);
  }

  ~WebServerImpl() {
    mg_stop(m_context);
  }

  int onBeginRequest(mg_connection* conn) {
    const mg_request_info* request_info = mg_get_request_info(conn);
    std::stringstream content;

    // Prepare the message we're going to send
    content << PACKAGE << " v" << VERSION;

    // Send HTTP reply to the client
    std::string contentStr = content.str();
    mg_printf(conn,
              "HTTP/1.1 200 OK\r\n"
              "Content-Type: text/plain\r\n"
              "Content-Length: %d\r\n"
              "\r\n"
              "%s",
              contentStr.size(), contentStr.c_str());

    // Returning non-zero tells mongoose that our function has replied to
    // the client, and mongoose should not send client any more data.
    return 1;
  }

private:
  mg_context* m_context;
  mg_callbacks m_callbacks;
};

static int begin_request_handler(mg_connection* conn)
{
  const mg_request_info* request_info = mg_get_request_info(conn);
  WebServer::WebServerImpl* webServer = reinterpret_cast<WebServer::WebServerImpl*>(request_info->user_data);
  return webServer->onBeginRequest(conn);
}

WebServer::WebServer()
  : m_impl(new WebServerImpl)
{
}

WebServer::~WebServer()
{
  delete m_impl;
}

}
