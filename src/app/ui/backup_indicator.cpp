// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/backup_indicator.h"

#include "app/ui/status_bar.h"
#include "ui/manager.h"

namespace app {

BackupIndicator::BackupIndicator() : m_timer(100), m_small(false), m_running(false)
{
  m_timer.Tick.connect([this] { onTick(); });
}

BackupIndicator::~BackupIndicator()
{
  m_timer.stop();
}

void BackupIndicator::start()
{
  m_running = true;
  m_timer.start();
}

void BackupIndicator::stop()
{
  m_running = false;
}

void BackupIndicator::onTick()
{
  if (!m_running) {
    StatusBar::instance()->showBackupIcon(StatusBar::BackupIcon::None);
    m_timer.stop();
    return;
  }

  StatusBar::instance()->showBackupIcon(m_small ? StatusBar::BackupIcon::Small :
                                                  StatusBar::BackupIcon::Normal);

  m_small = !m_small;
}

} // namespace app
