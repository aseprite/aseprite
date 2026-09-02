// Aseprite Network Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "net/http_request.h"

#include "base/debug.h"
#include "net/http_headers.h"
#include "net/http_response.h"

#include <algorithm>
#include <cctype>
#include <curl/curl.h>
#include <string>

namespace net {

namespace {

constexpr std::size_t kMaxResponseBytes = 8 * 1024 * 1024;
constexpr long kConnectTimeoutSec = 15;
constexpr long kTimeoutSec = 30;

bool starts_with_ci(const std::string& s, const char* prefix)
{
  const std::size_t n = std::char_traits<char>::length(prefix);
  if (s.size() < n)
    return false;
  for (std::size_t i = 0; i < n; ++i) {
    if (std::tolower(static_cast<unsigned char>(s[i])) !=
        std::tolower(static_cast<unsigned char>(prefix[i])))
      return false;
  }
  return true;
}

// First-party traffic must be HTTPS. Loopback HTTP is allowed for local
// testing via CUSTOM_WEBSITE_URL.
bool is_allowed_request_url(const std::string& url)
{
  if (url.find('\0') != std::string::npos || url.find('\n') != std::string::npos ||
      url.find('\r') != std::string::npos)
    return false;
  if (starts_with_ci(url, "https://"))
    return true;
  if (starts_with_ci(url, "http://127.0.0.1") || starts_with_ci(url, "http://localhost") ||
      starts_with_ci(url, "http://[::1]"))
    return true;
  return false;
}

} // namespace

class HttpRequestImpl {
public:
  HttpRequestImpl(const std::string& url)
    : m_curl(curl_easy_init())
    , m_headerlist(nullptr)
    , m_response(nullptr)
    , m_bytesWritten(0)
    , m_urlAllowed(is_allowed_request_url(url))
  {
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, &HttpRequestImpl::writeBodyCallback);
    curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(m_curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTPS | CURLPROTO_HTTP);
    curl_easy_setopt(m_curl, CURLOPT_REDIR_PROTOCOLS, 0L);
    curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT, kConnectTimeoutSec);
    curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, kTimeoutSec);
    curl_easy_setopt(m_curl, CURLOPT_MAXFILESIZE, long(kMaxResponseBytes));
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
    for (HttpHeaders::const_iterator it = headers.begin(), end = headers.end(); it != end; ++it) {
      tmp = it->first;
      tmp += ": ";
      tmp += it->second;

      m_headerlist = curl_slist_append(m_headerlist, tmp.c_str());
    }

    curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, m_headerlist);
  }

  bool send(HttpResponse& response)
  {
    if (!m_urlAllowed)
      return false;

    m_response = &response;
    m_bytesWritten = 0;
    int res = curl_easy_perform(m_curl);
    if (res != CURLE_OK)
      return false;

    long code;
    curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &code);
    m_response->setStatus(code);
    return true;
  }

  void abort()
  {
    curl_easy_setopt(m_curl, CURLOPT_TIMEOUT_MS, 1);
    curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT_MS, 1);
  }

private:
  std::size_t writeBody(char* ptr, std::size_t bytes)
  {
    ASSERT(m_response != NULL);
    if (m_bytesWritten + bytes > kMaxResponseBytes)
      return 0;
    m_response->write(ptr, bytes);
    m_bytesWritten += bytes;
    return bytes;
  }

  static std::size_t writeBodyCallback(char* ptr,
                                       std::size_t size,
                                       std::size_t nmemb,
                                       void* userdata)
  {
    HttpRequestImpl* req = reinterpret_cast<HttpRequestImpl*>(userdata);
    return req->writeBody(ptr, size * nmemb);
  }

  CURL* m_curl;
  curl_slist* m_headerlist;
  HttpResponse* m_response;
  std::size_t m_bytesWritten;
  bool m_urlAllowed;
};

HttpRequest::HttpRequest(const std::string& url) : m_impl(new HttpRequestImpl(url))
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

bool HttpRequest::send(HttpResponse& response)
{
  return m_impl->send(response);
}

void HttpRequest::abort()
{
  m_impl->abort();
}

} // namespace net
