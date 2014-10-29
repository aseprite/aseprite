/* Aseprite
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/find_widget.h"
#include "app/ini_file.h"
#include "app/load_widget.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/main_window.h"
#include "ui/ui.h"

#include <cstdio>

namespace app {

using namespace ui;

class AdvancedModeCommand : public Command {
public:
  AdvancedModeCommand();
  Command* clone() const override { return new AdvancedModeCommand(*this); }

protected:
  void onExecute(Context* context);
};

AdvancedModeCommand::AdvancedModeCommand()
  : Command("AdvancedMode",
            "Advanced Mode",
            CmdUIOnlyFlag)
{
}

void AdvancedModeCommand::onExecute(Context* context)
{
  // Switch advanced mode.
  MainWindow* mainWindow = App::instance()->getMainWindow();
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

  if (oldMode == MainWindow::NormalMode &&
      get_config_bool("AdvancedMode", "Warning", true)) {
    Key* key = KeyboardShortcuts::instance()->command(short_name());
    if (!key->accels().empty()) {
      base::UniquePtr<Window> window(app::load_widget<Window>("advanced_mode.xml", "advanced_mode_warning"));
      Widget* warning_label = app::find_widget<Widget>(window, "warning_label");
      Widget* donot_show = app::find_widget<Widget>(window, "donot_show");

      warning_label->setTextf("You can go back pressing \"%s\" key.",
        key->accels().front().toString().c_str());

      window->openWindowInForeground();

      set_config_bool("AdvancedMode", "Warning", !donot_show->isSelected());
    }
  }
}

Command* CommandFactory::createAdvancedModeCommand()
{
  return new AdvancedModeCommand;
}

} // namespace app
