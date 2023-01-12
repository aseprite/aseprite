// Aseprite
// Copyright (C) 2021-2023  Igara Studio S.A.
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

AsepriteUpdate::AsepriteUpdate(std::string version)
  : m_download(get_app_name(), version)
  , m_timer(500, this)
{
  okButton()->setEnabled(false);
  okButton()->Click.connect([this]() { });
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
  if ((m_download.status() != drm::Thread::Status::FINISHED) ||
      (m_installation && m_installation->status() != drm::Thread::Status::FINISHED)) {
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
    m_installation = std::make_unique<drm::InstallationThread>(package);
    m_installation->InstallationFailed.connect([this](drm::LicenseManager::InstallationException& e) {
      onInstallationFailed(e);
    });
    m_installation->InstallationStarted.connect([this](drm::Package& package) {
      onInstallationStarted(package);
    });
    m_installation->InstallationPhaseStarted.connect([this](drm::InstallationPhase phase) {
      onInstallationPhaseStarted(phase);
    });
    m_installation->InstallationPhaseSkipped.connect([this](drm::InstallationPhase phase) {
      onInstallationPhaseSkipped(phase);
    });
    m_installation->InstallationPhaseFinished.connect([this](drm::InstallationPhase phase) {
      onInstallationPhaseFinished(phase);
    });
    m_installation->InstallationProgress.connect([this](drm::InstallationPhase phase, int pctCompleted) {
      onInstallationProgress(pctCompleted);
    });
    m_installation->InstallationFinished.connect([this](drm::Package& package) {
      onInstallationFinished(package);
    });
    m_installation->start();
  });
}

void AsepriteUpdate::onInstallationPhaseStarted(drm::InstallationPhase phase)
{
  ui::execute_from_ui_thread([this, phase] {
    std::string msg = "";
    switch (phase) {
    case drm::InstallationPhase::CREATING_BACKUP:
      msg = "Creating backup...";
      break;
    case drm::InstallationPhase::UNPACKING_PACKAGE:
      msg = "Unpacking package...";
      break;
    case drm::InstallationPhase::INSTALLING_FILES:
      msg = "Installing files...";
      break;
    }
    if (!msg.empty()) log(msg);
  });
}

void AsepriteUpdate::onInstallationPhaseSkipped(drm::InstallationPhase phase)
{
  ui::execute_from_ui_thread([this, phase] {
    std::string msg = "";
    switch (phase) {
    case drm::InstallationPhase::CREATING_BACKUP:
      msg = "Creating backup skipped.";
      break;
    case drm::InstallationPhase::UNPACKING_PACKAGE:
      msg = "Unpacking package skipped.";
      break;
    case drm::InstallationPhase::INSTALLING_FILES:
      msg = "Installing files skipped.";
      break;
    }
    if (!msg.empty()) log(msg);
  });
}

void AsepriteUpdate::onInstallationPhaseFinished(drm::InstallationPhase phase)
{
  ui::execute_from_ui_thread([this, phase] {
    std::string msg;

    switch (phase) {
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
  });
}

void AsepriteUpdate::onInstallationStarted(drm::Package& package)
{
  ui::execute_from_ui_thread([this] {
    log("Installation process started...");
  });
}

void AsepriteUpdate::onInstallationProgress(int pctCompleted)
{
  ui::execute_from_ui_thread([this, pctCompleted] {
    progress()->setValue(pctCompleted);
  });
}

void AsepriteUpdate::onInstallationFinished(drm::Package& package)
{
  ui::execute_from_ui_thread([this] {
    log("Installation process finished!");
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
