// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/floating_window.h"

#include "app/ini_file.h"
#include "app/modules/gui.h"
#include "ui/display.h"
#include "ui/fit_bounds.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/system.h"

namespace app {

using namespace ui;

FloatingWindow::FloatingWindow(const std::string& title, const std::string& configSection)
  : Window(WithTitleBar, title)
  , m_configSection(configSection)
  , m_isEnabled(false)
{
  setAutoRemap(false);
  setWantFocus(false);

  m_isEnabled = get_config_bool(m_configSection.c_str(), "Enabled", false);

  initTheme();
}

FloatingWindow::~FloatingWindow()
{
  set_config_bool(m_configSection.c_str(), "Enabled", m_isEnabled);
}

void FloatingWindow::setEnabled(bool state)
{
  m_isEnabled = state;
  if (m_isEnabled) {
    if (!isVisible())
      openWindow();
  }
  else {
    if (isVisible())
      closeWindow(NULL);
  }
}

bool FloatingWindow::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {
    case kOpenMessage: {
      Manager* manager = this->manager();
      Display* mainDisplay = manager->display();

      gfx::Rect defaultBounds(mainDisplay->size() / 4);
      gfx::Rect mainWindow = manager->bounds();
      defaultBounds.x = mainWindow.x + (mainWindow.w - defaultBounds.w) / 2;
      defaultBounds.y = mainWindow.y + (mainWindow.h - defaultBounds.h) / 2;

      fit_bounds(mainDisplay, this, defaultBounds);
      load_window_pos(this, m_configSection.c_str(), false);
      invalidate();
      break;
    }

    case kCloseMessage: save_window_pos(this, m_configSection.c_str()); break;
  }

  return Window::onProcessMessage(msg);
}

void FloatingWindow::onClose(ui::CloseEvent& ev)
{
  m_isEnabled = false;
}

} // namespace app
