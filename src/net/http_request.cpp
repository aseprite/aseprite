// Aseprite Network Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "base/debug.h"
#include "net/http_headers.h"
#include "net/http_request.h"
#include "net/http_response.h"

#include <curl/curl.h>

#include <array>
#include <string_view>

namespace net {

bool is_valid_url(const std::string_view url)
{
  CURLU* h = curl_url();
  if (!h)
    return false;
  const auto rc = curl_url_set(h, CURLUPART_URL, url.data(), 0);
  curl_url_cleanup(h);
  return rc == CURLUE_OK;
}

std::string url_encode(const std::string_view text)
{
  std::string result;
  if (auto* escaped = curl_easy_escape(nullptr, text.data(), text.length())) {
    result = escaped;
    curl_free(escaped);
  }
  return result;
}

std::string url_decode(const std::string_view text)
{
  std::string result;
  int len;
  if (auto* unescaped = curl_easy_unescape(nullptr, text.data(), text.length(), &len)) {
    result = std::string(unescaped, len);
    curl_free(unescaped);
  }
  return result;
}

class HttpRequestImpl {
public:
  HttpRequestImpl(const std::string& url, const HttpRequest::Method method)
    : m_curl(curl_easy_init())
    , m_headerlist(nullptr)
    , m_response(nullptr)
  {
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, &HttpRequestImpl::writeBodyCallback);
    curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(m_curl, CURLOPT_ERRORBUFFER, m_errorBuffer.data());

    switch (method) {
      case HttpRequest::Method::PUT: curl_easy_setopt(m_curl, CURLOPT_UPLOAD, 1L); break;
      case HttpRequest::Method::OPTIONS:
        curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, "OPTIONS");
        break;
      case HttpRequest::Method::PATCH:
        curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, "PATCH");
        break;
      case HttpRequest::Method::POST: setPostFields(""); break;
      default:                        break;
    }
  }

  ~HttpRequestImpl()
  {
    if (m_headerlist)
      curl_slist_free_all(m_headerlist);

    curl_easy_cleanup(m_curl);
  }

  void setPostFields(const std::string& fields)
  {
    curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, fields.c_str());
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

  bool send(HttpResponse& response, int timeoutMs)
  {
    m_response = &response;
    curl_easy_setopt(m_curl, CURLOPT_TIMEOUT_MS, timeoutMs);
    CURLcode res = curl_easy_perform(m_curl);
    if (res != CURLE_OK) {
      if (!m_errorBuffer.empty())
        m_response->setError(std::string(m_errorBuffer.data()));
      else
        m_response->setError(curl_easy_strerror(res));
      return false;
    }

    long code;
    curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &code);
    m_response->setStatus(code);

    HttpHeaders headers;
    curl_header* prev = nullptr;
    while (curl_header* header = curl_easy_nextheader(m_curl, CURLH_HEADER, -1, prev)) {
      headers.setHeader(header->name, header->value);
      prev = header;
    }
    m_response->setHeaders(std::move(headers));

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
    m_response->write(ptr, bytes);
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
  std::array<char, CURL_ERROR_SIZE> m_errorBuffer;
};

HttpRequest::HttpRequest(const std::string& url, Method method)
  : m_impl(new HttpRequestImpl(url, method))
{
}

HttpRequest::~HttpRequest()
{
  delete m_impl;
}

void HttpRequest::setPostFields(const std::string& fields)
{
  m_impl->setPostFields(fields);
}

void HttpRequest::setHeaders(const HttpHeaders& headers)
{
  m_impl->setHeaders(headers);
}

bool HttpRequest::send(HttpResponse& response, int timeoutMs)
{
  return m_impl->send(response, timeoutMs);
}

void HttpRequest::abort()
{
  m_impl->abort();
}

} // namespace net
