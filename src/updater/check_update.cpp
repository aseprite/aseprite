// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "updater/check_update.h"

#include "base/bind.h"
#include "base/convert_to.h"
#include "net/http_headers.h"
#include "net/http_request.h"
#include "net/http_response.h"
#include "tinyxml.h"
#include "updater/user_agent.h"

#include <iostream>
#include <sstream>

namespace updater {

CheckUpdateResponse::CheckUpdateResponse()
  : m_type(Unknown)
  , m_waitDays(0.0)
{
}

CheckUpdateResponse::CheckUpdateResponse(const CheckUpdateResponse& other)
  : m_type(other.m_type)
  , m_version(other.m_version)
  , m_url(other.m_url)
  , m_waitDays(0.0)
{
}

CheckUpdateResponse::CheckUpdateResponse(const std::string& responseBody)
  : m_type(Unknown)
  , m_waitDays(0.0)
{
  TiXmlDocument doc;
  doc.Parse(responseBody.c_str());

  TiXmlHandle handle(&doc);
  TiXmlElement* xmlUpdate = handle.FirstChild("update").ToElement();
  if (!xmlUpdate) {
    // TODO show error?
    return;
  }

  const char* latest_attr = xmlUpdate->Attribute("latest");
  const char* version_attr = xmlUpdate->Attribute("version");
  const char* type_attr = xmlUpdate->Attribute("type");
  const char* url_attr = xmlUpdate->Attribute("url");
  const char* uuid_attr = xmlUpdate->Attribute("uuid");
  const char* waitdays_attr = xmlUpdate->Attribute("waitdays");

  if (latest_attr && strcmp(latest_attr, "1") == 0)
    m_type = NoUpdate;

  if (type_attr) {
    if (strcmp(type_attr, "critical") == 0)
      m_type = Critical;
    else if (strcmp(type_attr, "major") == 0)
      m_type = Major;
  }

  if (version_attr)
    m_version = version_attr;

  if (url_attr)
    m_url = url_attr;

  if (uuid_attr)
    m_uuid = uuid_attr;

  if (waitdays_attr)
    m_waitDays = base::convert_to<double>(std::string(waitdays_attr));
}

class CheckUpdate::CheckUpdateImpl
{
public:
  CheckUpdateImpl() { }
  ~CheckUpdateImpl() { }

  void abort()
  {
    // TODO impl
  }

  void checkNewVersion(const Uuid& uuid, const std::string& extraParams, CheckUpdateDelegate* delegate)
  {
    using namespace base;
    using namespace net;

    std::string url = UPDATE_URL;
    if (!uuid.empty()) {
      url += "&uuid=";
      url += uuid;
    }
    if (!extraParams.empty()) {
      url += "&";
      url += extraParams;
    }

    HttpRequest request(url);
    HttpHeaders headers;
    headers.setHeader("User-Agent", getUserAgent());
    request.setHeaders(headers);

    std::stringstream body;
    HttpResponse response(&body);
    request.send(response);

    DLOG("Checking updates: %s (User-Agent: %s)\n", url.c_str(), getUserAgent().c_str());
    DLOG("Response:\n--\n%s--\n", body.str().c_str());

    CheckUpdateResponse data(body.str());
    delegate->onResponse(data);
  }

};

CheckUpdate::CheckUpdate()
  : m_impl(new CheckUpdateImpl)
{
}

CheckUpdate::~CheckUpdate()
{
  delete m_impl;
}

void CheckUpdate::abort()
{
  m_impl->abort();
}

void CheckUpdate::checkNewVersion(const Uuid& uuid, const std::string& extraParams, CheckUpdateDelegate* delegate)
{
  m_impl->checkNewVersion(uuid, extraParams, delegate);
}

} // namespace updater
