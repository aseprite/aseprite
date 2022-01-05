// Aseprite
// Copyright (C) 2021  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "aseprite_update.h"
#include "base/time.h"
#include "base/trim_string.h"
#include "ui/label.h"
#include "ui/system.h"
#include "ver/info.h"

namespace app {

AsepriteUpdate::AsepriteUpdate(std::string version) : m_timer(500, this), m_download(get_app_name(), version)
{
  okButton()->setEnabled(false);
  okButton()->Click.connect([this](ui::Event&) {
                            });
  m_timer.Tick.connect([this] { this->onTimerTick(); });

  log(base::string_printf("Downloading Aseprite %s...", version.c_str()));
  m_download.DataReceived.connect(
    [this](long total, long now) {
      onDataReceived(total, now);
    }
  );
  m_download.DownloadFailed.connect(
    [this](drm::LicenseManager::DownloadException& e) {
      onDownloadFailed(e);
    }
  );
  m_download.DownloadFinished.connect(
    [this](drm::Package& package) {
      onDownloadFinished(package);
    }
  );
  m_download.start();
  m_timer.start();
}

void AsepriteUpdate::onBeforeClose(ui::CloseEvent& ev)
{
  if (m_download.status() != drm::Thread::Status::FINISHED ||
      m_installation && m_installation->status() != drm::Thread::Status::FINISHED) {
      log("Stopping, please wait...");
      ev.cancel();
  }
  if (!m_closing) {
    m_download.shutdown();
    if (m_installation) {
      m_installation->shutdown();
    }
  }
  m_closing = true;

}

void AsepriteUpdate::onDataReceived(long total, long now)
{
  ui::execute_from_ui_thread([this, total, now] {
    progress()->setValue(100 * now / total);
  });
}

void AsepriteUpdate::onDownloadFailed(drm::LicenseManager::DownloadException& e)
{
  ui::execute_from_ui_thread([this, e] {
    log(e.what());
  });
}

void AsepriteUpdate::onDownloadFinished(drm::Package& package)
{
  ui::execute_from_ui_thread([this, package] {
    log("Download finished!");
    m_installation.reset(new drm::InstallationThread(package));
    m_installation->InstallationFailed.connect([this](drm::LicenseManager::InstallationException& e) {
      onInstallationFailed(e);
    });
    m_installation->InstallationPhaseChanged.connect([this](drm::InstallationPhase oldPhase, drm::InstallationPhase phase) {
      onInstallationPhaseChanged(oldPhase, phase);
    });
    m_installation->start();
  });
}

void AsepriteUpdate::onInstallationPhaseChanged(drm::InstallationPhase oldPhase, drm::InstallationPhase phase)
{
  ui::execute_from_ui_thread([this, oldPhase, phase] {
    std::string msg;

    switch (oldPhase) {
    case drm::InstallationPhase::UNSPECIFIED:
      msg = "Installation process started...";
      break;
    case drm::InstallationPhase::SAVING_PACKAGE:
      msg = "Package saved!";
      break;
    case drm::InstallationPhase::CREATING_BACKUP:
      msg = "Backup created!";
      break;
    case drm::InstallationPhase::UNPACKING_PACKAGE:
      msg = "Package unpacked!";
      break;
    case drm::InstallationPhase::INSTALLING_FILES:
      msg = "Files installed!";
      break;
    }
    if (!msg.empty()) log(msg);

    msg = "";
    switch (phase) {
    case drm::InstallationPhase::SAVING_PACKAGE:
      msg = "Saving package...";
      break;
    case drm::InstallationPhase::CREATING_BACKUP:
      msg = "Creating backup...";
      break;
    case drm::InstallationPhase::UNPACKING_PACKAGE:
      msg = "Unpacking package...";
      break;
    case drm::InstallationPhase::INSTALLING_FILES:
      msg = "Installing files...";
      break;
    case drm::InstallationPhase::DONE:
      msg = "Installation process finished!";
      break;
    }
    if (!msg.empty()) log(msg);
  });
}

// TODO: Create a unique onFailure() method to handle any exception.
void AsepriteUpdate::onInstallationFailed(drm::LicenseManager::InstallationException& e)
{
  ui::execute_from_ui_thread([this, e] {
    log(e.what());
  });
}

void AsepriteUpdate::onTimerTick()
{
  if (m_closing && m_download.status() == drm::Thread::Status::FINISHED) {
    this->closeWindow(this);
  }
}

void AsepriteUpdate::log(std::string text)
{
  if (m_closing) return;

  base::trim_string(text, text);

  auto now = base::current_time();
  auto strNow = base::string_printf("%d-%02d-%02d %02d:%02d:%02d", now.year, now.month, now.day, now.hour, now.minute, now.second);
  logitems()->addChild(new ui::Label(strNow + " " + text));
  layout();
}

}