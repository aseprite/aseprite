// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef ENABLE_UPDATER

#include "app/check_update.h"

#include "app/check_update_delegate.h"
#include "app/pref/preferences.h"
#include "base/bind.h"
#include "base/convert_to.h"
#include "base/launcher.h"
#include "base/replace_string.h"
#include "base/version.h"
#include "ver/info.h"

#include <ctime>
#include <sstream>

static const int kMonitoringPeriod = 100;

namespace app {

class CheckUpdateBackgroundJob : public updater::CheckUpdateDelegate {
public:
  CheckUpdateBackgroundJob()
    : m_received(false) { }

  void abort() {
    m_checker.abort();
  }

  bool isReceived() const {
    return m_received;
  }

  void sendRequest(const updater::Uuid& uuid, const std::string& extraParams) {
    m_checker.checkNewVersion(uuid, extraParams, this);
  }

  const updater::CheckUpdateResponse& getResponse() const {
    return m_response;
  }

private:
  void onResponse(updater::CheckUpdateResponse& data) override {
    m_response = data;
    m_received = true;
  }

  bool m_received;
  updater::CheckUpdate m_checker;
  updater::CheckUpdateResponse m_response;
};

CheckUpdateThreadLauncher::CheckUpdateThreadLauncher(CheckUpdateDelegate* delegate)
  : m_delegate(delegate)
  , m_preferences(Preferences::instance())
  , m_doCheck(true)
  , m_received(false)
  , m_inits(m_preferences.updater.inits())
  , m_exits(m_preferences.updater.exits())
#ifdef _DEBUG
  , m_isDeveloper(true)
#else
  , m_isDeveloper(m_preferences.updater.isDeveloper())
#endif
  , m_timer(kMonitoringPeriod, NULL)
{
  // Get how many days we have to wait for the next "check for update"
  double waitDays = m_preferences.updater.waitDays();
  if (waitDays > 0.0) {
    // Get the date of the last "check for updates"
    time_t lastCheck = (time_t)m_preferences.updater.lastCheck();
    time_t now = std::time(NULL);

    // Verify if we are in the "WaitDays" period...
    if (now < lastCheck+int(double(60*60*24*waitDays)) &&
        now > lastCheck) {                               // <- Avoid broken clocks
      // So we do not check for updates.
      m_doCheck = false;
    }
  }

  // Minimal stats: number of initializations
  m_preferences.updater.inits(m_inits+1);
  m_preferences.save();
}

CheckUpdateThreadLauncher::~CheckUpdateThreadLauncher()
{
  if (m_timer.isRunning())
    m_timer.stop();

  if (m_thread) {
    if (m_bgJob)
      m_bgJob->abort();

    m_thread->join();
  }

  // Minimal stats: number of exits
  m_preferences.updater.exits(m_exits+1);
  m_preferences.save();
}

void CheckUpdateThreadLauncher::launch()
{
  // In this case we are in the "wait days" period, so we don't check
  // for updates.
  if (!m_doCheck) {
    showUI();
    return;
  }

  if (m_uuid.empty())
    m_uuid = m_preferences.updater.uuid();

  m_delegate->onCheckingUpdates();

  m_bgJob.reset(new CheckUpdateBackgroundJob);
  m_thread.reset(new base::thread(base::Bind<void>(&CheckUpdateThreadLauncher::checkForUpdates, this)));

  // Start a timer to monitoring the progress of the background job
  // executed in "m_thread". The "onMonitoringTick" method will be
  // called periodically by the GUI main thread.
  m_timer.Tick.connect(&CheckUpdateThreadLauncher::onMonitoringTick, this);
  m_timer.start();
}

bool CheckUpdateThreadLauncher::isReceived() const
{
  return m_received;
}

void CheckUpdateThreadLauncher::onMonitoringTick()
{
  // If we do not receive a response yet...
  if (!m_received)
    return;                     // Skip and wait the next call.

  // Depending on the type of update received
  switch (m_response.getUpdateType()) {

    case updater::CheckUpdateResponse::NoUpdate:
      // Clear
      m_preferences.updater.newVersion("");
      m_preferences.updater.newUrl("");
      break;

    case updater::CheckUpdateResponse::Critical:
    case updater::CheckUpdateResponse::Major:
      m_preferences.updater.newVersion(m_response.getLatestVersion());
      m_preferences.updater.newUrl(m_response.getUrl());
      break;
  }

  showUI();

  // Save the new UUID
  if (!m_response.getUuid().empty()) {
    m_uuid = m_response.getUuid();
    m_preferences.updater.uuid(m_uuid);
  }

  // Set the date of the last "check for updates" and the "WaitDays" parameter.
  m_preferences.updater.lastCheck((int)std::time(NULL));
  m_preferences.updater.waitDays(m_response.getWaitDays());

  // Save the config file right now
  m_preferences.save();

  // Stop the monitoring timer.
  m_timer.stop();
}

// This method is executed in a special thread to send the HTTP request.
void CheckUpdateThreadLauncher::checkForUpdates()
{
  // Add mini-stats in the request
  std::stringstream extraParams;
  extraParams << "inits=" << m_inits
              << "&exits=" << m_exits;

  if (m_isDeveloper)
    extraParams << "&dev=1";

  // Send the HTTP request to check for updates.
  m_bgJob->sendRequest(m_uuid, extraParams.str());

  if (m_bgJob->isReceived()) {
    m_received = true;
    m_response = m_bgJob->getResponse();
  }
}

void CheckUpdateThreadLauncher::showUI()
{
  std::string localVersionStr = get_app_version();
  base::replace_string(localVersionStr, "-x64", "");
  bool newVer = false;

  if (!m_preferences.updater.newVersion().empty()) {
    base::Version serverVersion(m_preferences.updater.newVersion());
    base::Version localVersion(localVersionStr);
    newVer = (localVersion < serverVersion);
  }

  if (newVer) {
    m_delegate->onNewUpdate(m_preferences.updater.newUrl(),
                            m_preferences.updater.newVersion());
  }
  else {
    // If the program was updated, reset the "exits" counter
    if (m_preferences.updater.currentVersion() != localVersionStr) {
      m_preferences.updater.currentVersion(localVersionStr);
      m_exits = m_inits;
    }

    m_delegate->onUpToDate();
  }
}

}

#endif // ENABLE_UPDATER
