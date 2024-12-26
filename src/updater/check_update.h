// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef UPDATER_CHECK_UPDATE_H_INCLUDED
#define UPDATER_CHECK_UPDATE_H_INCLUDED
#pragma once

#include "base/disable_copying.h"

#include <string>

namespace updater {

typedef std::string Uuid;

// Data received by a "check new version" request.
class CheckUpdateResponse {
public:
  enum Type {
    Unknown,
    NoUpdate, // No update available. You've the latest version.
    Critical, // There are critical bugs fixed.
    Major     // New major version.
  };

  CheckUpdateResponse();
  CheckUpdateResponse(const CheckUpdateResponse& other);
  CheckUpdateResponse(const std::string& responseBody);

  // Returns the type of the available update.
  Type getUpdateType() const { return m_type; }

  // Returns the latest version available to be downloaded.
  std::string getLatestVersion() const { return m_version; }

  // Returns the URL to download the new version.
  std::string getUrl() const { return m_url; }

  // Returns a UUID to be assigned to this user. This parameter is
  // returned only when the request is done without an existent
  // UUID.
  Uuid getUuid() const { return m_uuid; }

  // Returns the number of days that this client should wait for the
  // next "check for updates".
  double getWaitDays() const { return m_waitDays; }

private:
  Type m_type;
  std::string m_version;
  std::string m_url;
  Uuid m_uuid;
  double m_waitDays;
};

// Delegate called by CheckUpdate when the request to the server is
// done. It must be implemented by the client of CheckUpdate.
class CheckUpdateDelegate {
public:
  virtual ~CheckUpdateDelegate() {}

  // Called by CheckUpdate::checkNewVersion() when the response from
  // the "updates server" is received.
  virtual void onResponse(CheckUpdateResponse& data) = 0;
};

// Checks for new versions.
class CheckUpdate {
public:
  CheckUpdate();
  ~CheckUpdate();

  // Cancels any operation in progress. It is called automatically in
  // the destructor.
  void abort();

  // Sends a request to the "updates server" and calls the delegate
  // when the response is received.
  bool checkNewVersion(const Uuid& uuid,
                       const std::string& extraParams,
                       CheckUpdateDelegate* delegate);

private:
  class CheckUpdateImpl;
  CheckUpdateImpl* m_impl;

  DISABLE_COPYING(CheckUpdate);
};

} // namespace updater

#endif
