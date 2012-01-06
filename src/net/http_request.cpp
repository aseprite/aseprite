/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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
