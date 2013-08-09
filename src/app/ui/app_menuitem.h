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

#ifndef APP_UI_APP_MENUITEM_H_INCLUDED
#define APP_UI_APP_MENUITEM_H_INCLUDED

#include "ui/menu.h"

namespace app {
  class Command;
  class Params;

  // A widget that represent a menu item of the application.
  //
  // It's like a MenuItme, but it has a extra properties: the name of
  // the command to be executed when it's clicked (also that command is
  // used to check the availability of the command).
  class AppMenuItem : public ui::MenuItem {
  public:
    AppMenuItem(const char* text, Command* command, Params* params);
    ~AppMenuItem();

    Command* getCommand() { return m_command; }
    Params* getParams() { return m_params; }

  protected:
    bool onProcessMessage(ui::Message* msg) OVERRIDE;
    void onClick() OVERRIDE;

  private:
    Command* m_command;
    Params* m_params;
  };

} // namespace app

#endif
