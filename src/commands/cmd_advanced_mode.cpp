/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include "jinete/jinete.h"

#include "commands/command.h"
#include "app.h"
#include "core/cfg.h"
#include "modules/gui.h"
#include "widgets/colbar.h"
#include "widgets/statebar.h"
#include "widgets/tabs.h"

class AdvancedModeCommand : public Command
{
public:
  AdvancedModeCommand();
  Command* clone() const { return new AdvancedModeCommand(*this); }

protected:
  void execute(Context* context);

private:
  static bool advanced_mode;
};

bool AdvancedModeCommand::advanced_mode = false;

AdvancedModeCommand::AdvancedModeCommand()
  : Command("advanced_mode",
	    "Advanced Mode",
	    CmdUIOnlyFlag)
{
}

void AdvancedModeCommand::execute(Context* context)
{
  advanced_mode = !advanced_mode;

  if (advanced_mode) {
    jwidget_hide(app_get_toolbar());
    jwidget_hide(app_get_menubar());
    jwidget_hide(app_get_statusbar());
    jwidget_hide(app_get_colorbar());
    jwidget_hide(app_get_tabsbar());
  }
  else {
    jwidget_show(app_get_toolbar());
    jwidget_show(app_get_menubar());
    jwidget_show(app_get_statusbar());
    jwidget_show(app_get_colorbar());
    jwidget_show(app_get_tabsbar());
  }

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

      strcpy(warning, _("You can back pressing the \"%s\" key."));
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

Command* CommandFactory::create_advanced_mode_command()
{
  return new AdvancedModeCommand;
}
