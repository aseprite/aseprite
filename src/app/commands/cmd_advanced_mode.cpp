// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/pref/preferences.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/main_window.h"
#include "ui/ui.h"

#include "advanced_mode.xml.h"

#include <cstdio>

namespace app {

using namespace ui;

class AdvancedModeCommand : public Command {
public:
  AdvancedModeCommand();

protected:
  void onExecute(Context* context) override;
};

AdvancedModeCommand::AdvancedModeCommand()
  : Command(CommandId::AdvancedMode(), CmdUIOnlyFlag)
{
}

void AdvancedModeCommand::onExecute(Context* context)
{
  // Switch advanced mode.
  MainWindow* mainWindow = App::instance()->mainWindow();
  MainWindow::Mode oldMode = mainWindow->getMode();
  MainWindow::Mode newMode = oldMode;

  switch (oldMode) {
    case MainWindow::NormalMode:
      newMode = MainWindow::ContextBarAndTimelineMode;
      break;
    case MainWindow::ContextBarAndTimelineMode:
      newMode = MainWindow::EditorOnlyMode;
      break;
    case MainWindow::EditorOnlyMode:
      newMode = MainWindow::NormalMode;
      break;
  }

  mainWindow->setMode(newMode);

  auto& pref = Preferences::instance();

  if (oldMode == MainWindow::NormalMode &&
      pref.advancedMode.showAlert()) {
    KeyPtr key = KeyboardShortcuts::instance()->command(this->id().c_str());
    if (!key->accels().empty()) {
      app::gen::AdvancedMode window;

      window.warningLabel()->setTextf("You can go back pressing \"%s\" key.",
        key->accels().front().toString().c_str());

      window.openWindowInForeground();

      pref.advancedMode.showAlert(!window.donotShow()->isSelected());
    }
  }
}

Command* CommandFactory::createAdvancedModeCommand()
{
  return new AdvancedModeCommand;
}

} // namespace app
