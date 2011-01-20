/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#include "gui/jinete.h"

#include "commands/command.h"
#include "app.h"
#include "core/cfg.h"
#include "modules/gui.h"
#include "widgets/color_bar.h"
#include "widgets/statebar.h"
#include "widgets/tabs.h"

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

  app_get_toolbar()->setVisible(!advanced_mode);
  app_get_menubar()->setVisible(!advanced_mode);
  app_get_statusbar()->setVisible(!advanced_mode);
  app_get_colorbar()->setVisible(!advanced_mode);
  app_get_tabsbar()->setVisible(!advanced_mode);

  app_get_top_window()->remap_window();
  app_get_top_window()->dirty();
  
  if (advanced_mode &&
      get_config_bool("AdvancedMode", "Warning", true)) {
    JAccel accel = get_accel_to_execute_command(short_name());
    if (accel != NULL) {
      char warning[1024];
      char key[1024];
      char buf[1024];

      FramePtr window(load_widget("advanced_mode.xml", "advanced_mode_warning"));
      Widget* warning_label = find_widget(window, "warning_label");
      Widget* donot_show = find_widget(window, "donot_show");

      strcpy(warning, "You can back pressing the \"%s\" key.");
      jaccel_to_string(accel, key);
      sprintf(buf, warning, key);

      warning_label->setText(buf);

      window->open_window_fg();

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
