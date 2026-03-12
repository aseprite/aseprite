// Aseprite
// Copyright (C) 2025-present  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/i18n/strings.h"
#include "app/pref/preferences.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/main_window.h"
#include "ui/ui.h"

#include "advanced_mode.xml.h"
#include "app/context.h"

namespace app {

using namespace ui;

class AdvancedModeCommand : public Command {
public:
  AdvancedModeCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

AdvancedModeCommand::AdvancedModeCommand() : Command(CommandId::AdvancedMode())
{
}

bool AdvancedModeCommand::onEnabled(Context* context)
{
  return context->isUIAvailable();
}

void AdvancedModeCommand::onExecute(Context* context)
{
  // Switch advanced mode.
  MainWindow* mainWindow = App::instance()->mainWindow();
  MainWindow::Mode oldMode = mainWindow->getMode();
  MainWindow::Mode newMode = oldMode;

  switch (oldMode) {
    case MainWindow::NormalMode:                newMode = MainWindow::ContextBarAndTimelineMode; break;
    case MainWindow::ContextBarAndTimelineMode: newMode = MainWindow::EditorOnlyMode; break;
    case MainWindow::EditorOnlyMode:            newMode = MainWindow::NormalMode; break;
  }

  mainWindow->setMode(newMode);

  auto& pref = Preferences::instance();

  if (oldMode == MainWindow::NormalMode && pref.advancedMode.showAlert()) {
    KeyPtr key = KeyboardShortcuts::instance()->command(this->id().c_str());
    if (!key->shortcuts().empty()) {
      app::gen::AdvancedMode window;

      window.warningLabel()->setText(
        Strings::advanced_mode_quit(key->shortcuts().front().toString()));

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
