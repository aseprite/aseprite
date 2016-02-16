// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#error Not implemented

#include "webserver/webserver.h"

#include "base/bind.h"

#include <sstream>

namespace webserver {

static int begin_request_handler(mg_connection* conn);

class RequestResponseImpl : public IRequest
                          , public IResponse
{
public:
  RequestResponseImpl(mg_connection* conn, std::ostream& stream)
    : m_conn(conn)
    , m_requestInfo(mg_get_request_info(conn))
    , m_stream(stream)
    , m_code(200)
    , m_done(false)
    , m_contentType("text/plain")
  {
  }

  // IRequest implementation

  virtual const char* getRequestMethod() override {
    return m_requestInfo->request_method;
  }

  virtual const char* getUri() override {
    return m_requestInfo->uri;
  }

  virtual const char* getHttpVersion() override {
    return m_requestInfo->http_version;
  }

  virtual const char* getQueryString() override {
    return m_requestInfo->query_string;
  }

  // IResponse implementation

  virtual void setStatusCode(int code) override {
    m_code = code;
  }

  virtual void setContentType(const char* contentType) override {
    m_contentType = contentType;
  }

  virtual std::ostream& getStream() override {
    return m_stream;
  }

  virtual void sendFile(const char* path) override {
    mg_send_file(m_conn, path);
    m_done = true;
  }

  int getStatusCode() const {
    return m_code;
  }

  const char* getContentType() const {
    return m_contentType.c_str();
  }

  bool done() {
    return m_done;
  }

private:
  mg_connection* m_conn;
  const mg_request_info* m_requestInfo;
  std::ostream& m_stream;
  int m_code;
  bool m_done;
  std::string m_contentType;
};

class WebServer::WebServerImpl
{
public:
  WebServerImpl(IDelegate* delegate)
    : m_delegate(delegate) {
    const char* options[] = {
      "listening_ports", "10453",
      NULL
    };

    memset(&m_callbacks, 0, sizeof(m_callbacks));
    m_callbacks.begin_request = &begin_request_handler;

    m_context = mg_start(&m_callbacks, (void*)this, options);
  }

  ~WebServerImpl() {
    mg_stop(m_context);
  }

  std::string getName() const {
    std::string name;
    name = "mongoose ";
    name += mg_version();
    return name;
  }

  int onBeginRequest(mg_connection* conn) {
    std::stringstream body;
    RequestResponseImpl rr(conn, body);
    m_delegate->onProcessRequest(&rr, &rr);

    if (rr.done())
      return 1;

    // Send HTTP reply to the client
    std::string bodyStr = body.str();
    std::stringstream headers;

    headers << "HTTP/1.1 "
            << rr.getStatusCode() << " "
            << getStatusCodeString(rr.getStatusCode()) << "\r\n"
            << "Server: mongoose/" << mg_version() << "\r\n"
            << "Content-Type: " << rr.getContentType() << "\r\n"
            << "Content-Length: " << bodyStr.size() << "\r\n"
            << "Access-Control-Allow-Origin: *\r\n"
            << "\r\n";

    std::string headersStr = headers.str();

    mg_write(conn, (const void*)headersStr.c_str(), headersStr.size());
    mg_write(conn, (const void*)bodyStr.c_str(), bodyStr.size());

    // Returning non-zero tells mongoose that our function has replied to
    // the client, and mongoose should not send client any more data.
    return 1;
  }

private:
  const char* getStatusCodeString(int code) {
    switch (code) {
      case 100: return "Continue";
      case 101: return "Switching Protocols";
      case 102: return "Processing";
      case 200: return "OK";
      case 201: return "Created";
      case 202: return "Accepted";
      case 203: return "Non-Authoritative Information";
      case 204: return "No Content";
      case 205: return "Reset Content";
      case 206: return "Partial Content";
      case 207: return "Multi-Status";
      case 208: return "Already Reported";
      case 226: return "IM Used";
      case 300: return "Multiple Choices";
      case 301: return "Moved Permanently";
      case 302: return "Found";
      case 303: return "See Other";
      case 304: return "Not Modified";
      case 305: return "Use Proxy";
      case 306: return "Reserved";
      case 307: return "Temporary Redirect";
      case 308: return "Permanent Redirect";
      case 400: return "Bad Request";
      case 401: return "Unauthorized";
      case 402: return "Payment Required";
      case 403: return "Forbidden";
      case 404: return "Not Found";
      case 405: return "Method Not Allowed";
      case 406: return "Not Acceptable";
      case 407: return "Proxy Authentication Required";
      case 408: return "Request Timeout";
      case 409: return "Conflict";
      case 410: return "Gone";
      case 411: return "Length Required";
      case 412: return "Precondition Failed";
      case 413: return "Request Entity Too Large";
      case 414: return "Request-URI Too Long";
      case 415: return "Unsupported Media Type";
      case 416: return "Requested Range Not Satisfiable";
      case 417: return "Expectation Failed";
      case 422: return "Unprocessable Entity";
      case 423: return "Locked";
      case 424: return "Failed Dependency";
      case 426: return "Upgrade Required";
      case 428: return "Precondition Required";
      case 429: return "Too Many Requests";
      case 431: return "Request Header Fields Too Large";
      case 500: return "Internal Server Error";
      case 501: return "Not Implemented";
      case 502: return "Bad Gateway";
      case 503: return "Service Unavailable";
      case 504: return "Gateway Timeout";
      case 505: return "HTTP Version Not Supported";
      case 506: return "Variant Also Negotiates (Experimental)";
      case 507: return "Insufficient Storage";
      case 508: return "Loop Detected";
      case 510: return "Not Extended";
      case 511: return "Network Authentication Required";
      default: return "Unassigned";
    }
  }

  IDelegate* m_delegate;
  mg_context* m_context;
  mg_callbacks m_callbacks;
};

static int begin_request_handler(mg_connection* conn)
{
  const mg_request_info* request_info = mg_get_request_info(conn);
  WebServer::WebServerImpl* webServer =
    reinterpret_cast<WebServer::WebServerImpl*>(request_info->user_data);

  return webServer->onBeginRequest(conn);
}

WebServer::WebServer(IDelegate* delegate)
  : m_impl(new WebServerImpl(delegate))
{
}

WebServer::~WebServer()
{
  delete m_impl;
}

std::string WebServer::getName() const
{
  return m_impl->getName();
}

}
