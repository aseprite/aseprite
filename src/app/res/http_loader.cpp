// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/res/http_loader.h"

#include "base/bind.h"
#include "base/fs.h"
#include "base/fstream_path.h"
#include "base/replace_string.h"
#include "base/scoped_value.h"
#include "base/string.h"
#include "net/http_request.h"
#include "net/http_response.h"
#include "ver/info.h"

#include <fstream>

namespace app {

HttpLoader::HttpLoader(const std::string& url)
  : m_url(url)
  , m_done(false)
  , m_request(nullptr)
  , m_thread(base::Bind<void>(&HttpLoader::threadHttpRequest, this))
{
}

HttpLoader::~HttpLoader()
{
  m_thread.join();
}

void HttpLoader::abort()
{
  if (m_request)
    m_request->abort();
}

void HttpLoader::threadHttpRequest()
{
  try {
    base::ScopedValue<bool> scoped(m_done, false, true);

    LOG("HTTP: Sending http request to %s\n", m_url.c_str());

    std::string dir = base::join_path(base::get_temp_path(), get_app_name());
    base::make_all_directories(dir);

    std::string fn = m_url;
    base::replace_string(fn, ":", "-");
    base::replace_string(fn, "/", "-");
    base::replace_string(fn, "?", "-");
    base::replace_string(fn, "&", "-");
    fn = base::join_path(dir, fn);

    std::ofstream output(FSTREAM_PATH(fn), std::ofstream::binary);
    m_request = new net::HttpRequest(m_url);
    net::HttpResponse response(&output);
    if (m_request->send(response) &&
        response.status() == 200) {
      m_filename = fn;
    }

    LOG("HTTP: Response: %d\n", response.status());
  }
  catch (const std::exception& e) {
    LOG(ERROR) << "HTTP: Unexpected exception sending http request: " << e.what() << "\n";
  }
  catch (...) {
    LOG(ERROR) << "HTTP: Unexpected unknown exception sending http request\n";
  }

  delete m_request;
  m_request = nullptr;
}

} // namespace app
