// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef ENABLE_UPDATER

#include "app/check_update.h"

#include "app/check_update_delegate.h"
#include "app/ini_file.h"
#include "base/bind.h"
#include "base/convert_to.h"
#include "base/launcher.h"

#include <ctime>
#include <sstream>

static const int kMonitoringPeriod = 100;

namespace app {

class CheckUpdateBackgroundJob : public updater::CheckUpdateDelegate
{
public:
  CheckUpdateBackgroundJob()
    : m_canceled(false)
    , m_received(false) { }

  virtual ~CheckUpdateBackgroundJob() { }

  void cancel()
  {
    m_canceled = true;
  }

  bool isCanceled() const
  {
    return m_canceled;
  }

  bool isReceived() const
  {
    return m_received;
  }

  void sendRequest(const updater::Uuid& uuid, const std::string& extraParams)
  {
    m_checker.checkNewVersion(uuid, extraParams, this);
  }

  const updater::CheckUpdateResponse& getResponse() const
  {
    return m_response;
  }

private:

  // CheckUpdateDelegate implementation
  virtual void onResponse(updater::CheckUpdateResponse& data)
  {
    m_response = data;
    m_received = true;
  }

  bool m_canceled;
  bool m_received;
  updater::CheckUpdate m_checker;
  updater::CheckUpdateResponse m_response;
};

CheckUpdateThreadLauncher::CheckUpdateThreadLauncher(CheckUpdateDelegate* delegate)
  : m_delegate(delegate)
  , m_doCheck(true)
  , m_received(false)
  , m_inits(get_config_int("Updater", "Inits", 0))
  , m_exits(get_config_int("Updater", "Exits", 0))
#ifdef _DEBUG
  , m_isDeveloper(true)
#else
  , m_isDeveloper(get_config_bool("Updater", "IsDeveloper", false))
#endif
  , m_timer(kMonitoringPeriod, NULL)
{
  // Get how many days we have to wait for the next "check for update"
  int waitDays = get_config_int("Updater", "WaitDays", 0);
  if (waitDays > 0) {
    // Get the date of the last "check for updates"
    time_t lastCheck = (time_t)get_config_int("Updater", "LastCheck", 0);
    time_t now = std::time(NULL);

    // Verify if we are in the "WaitDays" period...
    if (now < lastCheck+60*60*24*waitDays &&
        now > lastCheck) {                               // <- Avoid broken clocks
      // So we do not check for updates.
      m_doCheck = false;
    }
  }

  // Minimal stats: number of initializations
  set_config_int("Updater", "Inits", get_config_int("Updater", "Inits", 0)+1);
  flush_config_file();
}

CheckUpdateThreadLauncher::~CheckUpdateThreadLauncher()
{
  if (m_timer.isRunning())
    m_timer.stop();

  if (m_thread) {
    if (m_bgJob)
      m_bgJob->cancel();

    m_thread->join();
  }

  // Minimal stats: number of exits
  set_config_int("Updater", "Exits", get_config_int("Updater", "Exits", 0)+1);
  flush_config_file();
}

void CheckUpdateThreadLauncher::launch()
{
  // In this case we are in the "wait days" period, so we don't check
  // for updates.
  if (!m_doCheck)
    return;

  if (m_uuid.empty())
    m_uuid = get_config_string("Updater", "Uuid", "");

  m_delegate->onCheckingUpdates();

  m_bgJob.reset(new CheckUpdateBackgroundJob);
  m_thread.reset(new base::thread(Bind<void>(&CheckUpdateThreadLauncher::checkForUpdates, this)));

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
      m_delegate->onUpToDate();
      break;

    case updater::CheckUpdateResponse::Critical:
    case updater::CheckUpdateResponse::Major:
      m_delegate->onNewUpdate(
        m_response.getUrl(),
        base::convert_to<std::string>(m_response.getLatestVersion()));
      break;
  }

  // Save the new UUID
  if (!m_response.getUuid().empty()) {
    m_uuid = m_response.getUuid();
    set_config_string("Updater", "Uuid", m_uuid.c_str());
  }

  // Set the date of the last "check for updates" and the "WaitDays" parameter.
  set_config_int("Updater", "LastCheck", (int)std::time(NULL));
  set_config_int("Updater", "WaitDays", m_response.getWaitDays());

  // Save the config file right now
  flush_config_file();

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

}

#endif // ENABLE_UPDATER
