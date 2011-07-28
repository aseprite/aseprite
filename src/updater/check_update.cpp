/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#define UPDATE_URL	WEBSITE "update/?xml=1"

namespace updater {

CheckUpdateResponse::CheckUpdateResponse()
  : m_type(Unknown)
  , m_waitDays(0)
{
}

CheckUpdateResponse::CheckUpdateResponse(const CheckUpdateResponse& other)
  : m_type(other.m_type)
  , m_version(other.m_version)
  , m_url(other.m_url)
  , m_waitDays(0)
{
}
    
CheckUpdateResponse::CheckUpdateResponse(const std::string& responseBody)
  : m_type(Unknown)
  , m_waitDays(0)
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
    m_version = base::convert_to<base::Version>(std::string(version_attr));

  if (url_attr)
    m_url = url_attr;

  if (uuid_attr)
    m_uuid = uuid_attr;

  if (waitdays_attr)
    m_waitDays = base::convert_to<int>(std::string(waitdays_attr));
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

}
