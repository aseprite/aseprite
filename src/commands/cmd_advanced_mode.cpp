/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#include "config.h"

#include "app.h"
#include "app/find_widget.h"
#include "app/load_widget.h"
#include "commands/command.h"
#include "ini_file.h"
#include "modules/gui.h"
#include "ui/gui.h"
#include "widgets/color_bar.h"
#include "widgets/main_menu_bar.h"
#include "widgets/main_window.h"
#include "widgets/status_bar.h"
#include "widgets/tabs.h"
#include "widgets/toolbar.h"

#include <cstdio>

using namespace ui;

class AdvancedModeCommand : public Command
{
public:
  AdvancedModeCommand();
  Command* clone() const { return new AdvancedModeCommand(*this); }

protected:
  void onExecute(Context* context);

private:
  static bool advanced_mode;
};

bool AdvancedModeCommand::advanced_mode = false;

AdvancedModeCommand::AdvancedModeCommand()
  : Command("AdvancedMode",
            "Advanced Mode",
            CmdUIOnlyFlag)
{
}

void AdvancedModeCommand::onExecute(Context* context)
{
  advanced_mode = !advanced_mode;

  MainWindow* mainWindow = App::instance()->getMainWindow();
  mainWindow->getMenuBar()->setVisible(!advanced_mode);
  mainWindow->getTabsBar()->setVisible(!advanced_mode);
  ToolBar::instance()->setVisible(!advanced_mode);
  StatusBar::instance()->setVisible(!advanced_mode);
  ColorBar::instance()->setVisible(!advanced_mode);

  mainWindow->remap_window();
  mainWindow->invalidate();

  if (advanced_mode &&
      get_config_bool("AdvancedMode", "Warning", true)) {
    JAccel accel = get_accel_to_execute_command(short_name());
    if (accel != NULL) {
      char warning[1024];
      char key[1024];
      char buf[1024];

      UniquePtr<Window> window(app::load_widget<Window>("advanced_mode.xml", "advanced_mode_warning"));
      Widget* warning_label = app::find_widget<Widget>(window, "warning_label");
      Widget* donot_show = app::find_widget<Widget>(window, "donot_show");

      strcpy(warning, "You can back pressing the \"%s\" key.");
      jaccel_to_string(accel, key);
      std::sprintf(buf, warning, key);

      warning_label->setText(buf);

      window->openWindowInForeground();

      set_config_bool("AdvancedMode", "Warning", !donot_show->isSelected());
    }
  }
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createAdvancedModeCommand()
{
  return new AdvancedModeCommand;
}
