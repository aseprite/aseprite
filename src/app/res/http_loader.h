// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_RES_HTTP_LOADER_H_INCLUDED
#define APP_RES_HTTP_LOADER_H_INCLUDED
#pragma once

#include "base/thread.h"
#include <string>

namespace app {

  class HttpLoader {
  public:
    HttpLoader(const std::string& url);
    ~HttpLoader();

    void cancel();
    bool isDone() const { return m_done; }
    std::string filename() const { return m_filename; }

  private:
    void threadHttpRequest();

    std::string m_url;
    bool m_done;
    bool m_cancel;
    base::thread m_thread;
    std::string m_filename;
  };

} // namespace app

#endif
