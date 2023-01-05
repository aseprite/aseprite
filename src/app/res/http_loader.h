// Aseprite
// Copyright (C) 2022-2023  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_RES_HTTP_LOADER_H_INCLUDED
#define APP_RES_HTTP_LOADER_H_INCLUDED
#pragma once

#include <atomic>
#include <string>
#include <thread>

namespace net {
  class HttpRequest;
}

namespace app {

  class HttpLoader {
  public:
    HttpLoader(const std::string& url);
    ~HttpLoader();

    void abort();
    bool isDone() const { return m_done; }
    std::string filename() const { return m_filename; }

  private:
    void threadHttpRequest();

    std::string m_url;
    std::atomic<bool> m_done;
    net::HttpRequest* m_request;
    std::thread m_thread;
    std::string m_filename;
  };

} // namespace app

#endif
