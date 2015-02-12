// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "net/http_request.h"

#include "net/http_headers.h"
#include "net/http_response.h"

#include <curl/curl.h>

namespace net {

class HttpRequestImpl
{
public:
  HttpRequestImpl(const std::string& url)
    : m_curl(curl_easy_init())
    , m_headerlist(NULL)
    , m_response(NULL)
  {
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, &HttpRequestImpl::writeBodyCallback);
    curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
  }

  ~HttpRequestImpl()
  {
    if (m_headerlist)
      curl_slist_free_all(m_headerlist);

    curl_easy_cleanup(m_curl);
  }

  void setHeaders(const HttpHeaders& headers)
  {
    if (m_headerlist) {
      curl_slist_free_all(m_headerlist);
      m_headerlist = NULL;
    }

    std::string tmp;
    for (HttpHeaders::const_iterator it=headers.begin(), end=headers.end(); it!=end; ++it) {
      tmp = it->first;
      tmp += ": ";
      tmp += it->second;

      m_headerlist = curl_slist_append(m_headerlist, tmp.c_str());
    }

    curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, m_headerlist);
  }

  void send(HttpResponse& response)
  {
    m_response = &response;
    curl_easy_perform(m_curl);
  }

  void abort()
  {
    // TODO
  }

private:
  size_t writeBody(char* ptr, size_t bytes)
  {
    ASSERT(m_response != NULL);
    m_response->write(ptr, bytes);
    return bytes;
  }

  static size_t writeBodyCallback(char* ptr, size_t size, size_t nmemb, void* userdata)
  {
    HttpRequestImpl* req = reinterpret_cast<HttpRequestImpl*>(userdata);
    return req->writeBody(ptr, size*nmemb);
  }

  CURL* m_curl;
  curl_slist* m_headerlist;
  HttpResponse* m_response;
};

HttpRequest::HttpRequest(const std::string& url)
  : m_impl(new HttpRequestImpl(url))
{
}

HttpRequest::~HttpRequest()
{
  delete m_impl;
}

void HttpRequest::setHeaders(const HttpHeaders& headers)
{
  m_impl->setHeaders(headers);
}

void HttpRequest::send(HttpResponse& response)
{
  m_impl->send(response);
}

void HttpRequest::abort()
{
  m_impl->abort();
}

} // namespace net
