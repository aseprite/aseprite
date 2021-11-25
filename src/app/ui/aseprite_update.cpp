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
  if (m_download.status() != drm::DownloadThread::Status::FINISHED) {
      log("Stopping, please wait...");
      ev.cancel();
  }
  if (!m_closing) {
    m_download.shutdown();
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
    drm::LicenseManager::instance()->installPackage(package);
  });
}

void AsepriteUpdate::onTimerTick()
{
  if (m_closing && m_download.status() == drm::DownloadThread::Status::FINISHED) {
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